#include "pch.h"
#include <algorithm>
#include "script_host.h"
#include "lua_resource.h"
#include "lua_om.h"
#include "lua_jobs.h"
//#include "lua_worker_scheduler.h"
#include "lua_basic_types.h"
#include "lua_noise.h"
#include "simulation/simulation.h"
#include "simulation/jobs/multi_path_finder.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/goto_location.h"
#include "resources/res_manager.h"
#include "om/entity.h"
#include "om/om_alloc.h"
#include "om/stonehearth.h"
#include "csg/voronoi_map.h"
#include "csg/util.h"

#define DEFINE_ALL_COMPONENTS
#include "om/all_components.h"

namespace fs = ::boost::filesystem;
namespace po = boost::program_options;
extern po::variables_map configvm;

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace luabind;


static std::string last_error;
static std::string last_error_traceback;

void dumpTable(lua_State* L, int tableStackOffset)
{
   lua_pushnil(L);  /* first key */
   while (lua_next(L, tableStackOffset - 1) != 0) {
      if (std::string(lua_typename(L, lua_type(L, -2))) == "string") {
         printf("%s - %s\n",
            lua_tolstring(L, -2, NULL),
            lua_typename(L, lua_type(L, -1)));
      } else {
         /* uses 'key' (at index -2) and 'value' (at index -1) */
         printf("%s - %s\n",
            lua_typename(L, lua_type(L, -2)),
            lua_typename(L, lua_type(L, -1)));
      }
      /* removes 'value'; keeps 'key' for next iteration */
      lua_pop(L, 1);
   }
}


void stackDump (lua_State *L) {
   int i;
   int top = lua_gettop(L);
   printf("current lua stack ---------------> ");
   for (i = 1; i <= top; i++) {  /* repeat for each level */
      int t = lua_type(L, i);
      switch (t) {
    
         case LUA_TSTRING:  /* strings */
         printf("'%s'", lua_tostring(L, i));
         break;
    
         case LUA_TBOOLEAN:  /* booleans */
         printf(lua_toboolean(L, i) ? "true" : "false");
         break;
    
         case LUA_TNUMBER:  /* numbers */
         printf("%g", lua_tonumber(L, i));
         break;
    
         default:  /* other values */
         printf("%s", lua_typename(L, t));
         break;
    
      }
      printf("  ");  /* put a separator */
   }
   printf("\n");  /* end the listing */
   }

ScriptHost* ScriptHost::singleton_;

ScriptHost& ScriptHost::GetInstance()
{
   ASSERT(singleton_);
   return *singleton_;
}

void* ScriptHost::LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize)
{
   if (nsize == 0) {
      free(ptr);
      return NULL;
   }
   return realloc(ptr, nsize);
}


ScriptHost::ScriptHost(lua_State* L, std::string scriptRoot) :
   L_(L)
{
   ASSERT(!singleton_);
   singleton_ = this;

   // Load all the scripts...
   InitEnvironment(scriptRoot);
   LoadScript((fs::path(scriptRoot) / "radiant" / "api.lua").string());
   LoadGameScript();
}

ScriptHost::~ScriptHost()
{
   ASSERT(singleton_);
   singleton_ = nullptr;
}

void ScriptHost::ReportError(luabind::object errorobj)
{
   std::string error = object_cast<std::string>(errorobj);
   LOG(WARNING) << "--------------------------------------------------------";
   LOG(WARNING) << error;
}

static int sh_pcall_callback_fun(lua_State* L)
{
   if (!lua_isstring(L, 1))  /* 'message' not a string? */
      return 1;  /* keep it intact */

   last_error = lua_tostring(L, 1);

   LOG(WARNING) << lua_tostring(L, 1);
   lua_getfield(L, LUA_GLOBALSINDEX, "debug");
   if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      LOG(WARNING) << "aborting traceback.  expected table for debug.";
      return 1;
   }
   lua_getfield(L, -1, "traceback");
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 2);
      LOG(WARNING) << "aborting traceback.  expected function for traceback.";
      return 1;
   }
   LOG(WARNING) << "generating traceback...";
   lua_call(L, 0, 1);  /* call debug.traceback */
   last_error_traceback = std::string(lua_tostring(L, -1));
   lua_pop(L, 1);

   return 1;
}

int GetConfigOptions(lua_State* L)
{
   object arg1(from_stack(L, -1));

   std::string name = object_cast<std::string>(arg1);
   const boost::any &value = configvm[name].value();
   if (value.type() == typeid(bool)) {
      lua_pushboolean(L, boost::any_cast<bool>(value));
      return 1;
   }
   return 0;
}

void ScriptHost::InitEnvironment(std::string root)
{
   int ii = lua_gettop(L_);

   set_pcall_callback(sh_pcall_callback_fun);

   module(L_) [
      class_<ScriptHost>("RadiantNative")
         .def("lookup_resource",          &ScriptHost::LookupResource)
         .def("report_error",             &ScriptHost::ReportError)
         .def("create_entity",            &ScriptHost::CreateEntityRef)
         .def("get_entity",               &ScriptHost::GetEntityRef)
         .def("create_grid",              &ScriptHost::CreateGrid)
         .def("destroy_entity",           &ScriptHost::DestroyEntity)
         .def("create_multi_path_finder", &ScriptHost::CreateMultiPathFinder)
         .def("create_path_finder",       &ScriptHost::CreatePathFinder)
         .def("create_follow_path",       &ScriptHost::CreateFollowPath)
         .def("create_goto_location",     &ScriptHost::CreateGotoLocation)
         .def("create_goto_entity",       &ScriptHost::CreateGotoEntity)
         .def("log",                      &ScriptHost::Log)
         .def("assert_failed",            &ScriptHost::AssertFailed)
         .def("unstick",                  &ScriptHost::Unstick)
   ];
   lua_register(L_, "get_config_option", GetConfigOptions);

   LuaBasicTypes::RegisterType(L_);
   LuaResource::RegisterType(L_);
   LuaObjectModel::RegisterType(L_);
   LuaJobs::RegisterType(L_);
   LuaNoise::RegisterType(L_);

   module(L_) [
      csg::RegisterLuaType(L_),
      csg::VoronoiMap::RegisterLuaType(L_, "VoronoiMap"),
      csg::VoronoiMapSite::RegisterLuaType(L_, "VoronoiMapSite")
   ];

   //LuaWorkerScheduler::RegisterType(L_);

   //globals(L_)["native"] = object(L_, this);

   //globals(L_)["package"]["path"] = "./scripts/?.lua;./scripts/lib/?.lua";
   //globals(L_)["package"]["cpath"] = "./scripts/?.lua;./scripts/clib/?.dll";

   globals(L_)["package"]["path"] = "./?.lua";
   globals(L_)["package"]["cpath"] = "";
}

void ScriptHost::LoadGameScript()
{
   std::ifstream in;
   std::string uri = configvm["game.script"].as<std::string>();
   if (!resources::ResourceManager2::GetInstance().OpenResource(uri, in)) {
      LOG(WARNING) << "could not open game script file '" << uri << "'.";
      return;
   }
   std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

   if (luaL_dostring(L_, str.c_str()) != 0) {
      OnError(lua_tostring(L_, -1));
      return;
   }
   try {
      game_ = luabind::object(luabind::from_stack(L_, -1));
      call_function<void>(game_["init"], game_, this);
   } CATCH_LUA_ERROR("initializing the environment");
}

void ScriptHost::CreateNew()
{
   try {
      luabind::call_function<void>(game_["create_new"], game_);
   } CATCH_LUA_ERROR("initializing environment");

#if 0
   // Run each scenario once...
   object options = luabind::newtable(L_);
   options["box"] = object(L_, math3d::aabb(math3d::point3(-20, 0, -20), math3d::point3(20, 20, 20)));
   for (const auto& entry : scenarios_) {
      LOG(WARNING) << "creating new scenario " << entry.first << ".";
      call_function<object>(entry.second, "construct", options);
      LOG(WARNING) << "done.";
   }
#endif
}

void ScriptHost::Update(int interval, int& currentGameTime)
{
   PROFILE_BLOCK();

   /*
   for (dm::ObjectRef c : allocedComponents_) {
      dm::ObjectPtr obj = c.lock();
      if (obj) {
         try {
            om::Entity& e = std::dynamic_pointer_cast<om::Component>(obj)->GetEntity();
            call_function<void>(reactor_["on_component_allocated"],
                                reactor_,
                                e.GetEntityId(),
                                om::GetComponentNameLower(obj));
         } CATCH_LUA_ERROR("starting reactor");
      }
   }  
   allocedComponents_.clear();
   */

   //CallEnvironment("update");
   try {
      currentGameTime = luabind::call_function<int>(game_["update"], game_, interval);
   } CATCH_LUA_ERROR("update...");
}

void ScriptHost::Idle(platform::timer &timer)
{
   bool finished = false;
   while (!timer.expired() && !finished) {
      finished = (lua_gc(L_, LUA_GCSTEP, 1) != 0);
   }
}

std::string ScriptHost::DoAction(const tesseract::protocol::DoAction& msg)
{
   luabind::object args[4];
   std::string action = msg.action();
   om::EntityId entity = msg.entity();

   ASSERT(msg.args().size() < ARRAY_SIZE(args));
   for (int i = 0; i < msg.args_size(); i++) {
      args[i] = ConvertArg(msg.args(i));
   }
   try {
      return luabind::call_function<std::string>(game_["execute_command"], game_, action, args[0], args[1], args[2], args[3]);
   } CATCH_LUA_ERROR("initializing environment");
   return "{ \"error\" : \"lua interpreter error\" }";
}

// xxx: Once we've converted the selectino object to a boost<any>, use
// selection::ToLuaObject
luabind::object ScriptHost::ConvertArg(const Protocol::Selection& arg)
{
   om::EntityId entityId = arg.entity();
   if (entityId != 0) {
      auto j = entityMap_.find(arg.entity());
      if (j != entityMap_.end()) {
         return luabind::object(L_, om::EntityRef(j->second));
      }
      return luabind::object(L_, om::EntityRef());
   }
   if (arg.has_block()) {
      return luabind::object(L_, math3d::ipoint3(arg.block()));
   }
   if (arg.has_bounds()) {
      return luabind::object(L_, math3d::ibounds3(arg.bounds()));
   }
   if (arg.has_string()) {
      return luabind::object(L_, arg.string());
   }
   if (arg.has_number()) {
      return luabind::object(L_, arg.number());
   }

#define OM_OBJECT(Cls, lower) \
   if (objectType == om::Cls::DmType) { \
      auto obj = Simulation::GetInstance().GetStore().FetchObject<om::Cls>(objectId); \
      return luabind::object(L_, obj); \
   }

   if (arg.has_object()) {
      dm::ObjectId objectId = arg.object();
      dm::ObjectType objectType = arg.object_type();

      OM_ALL_OBJECTS
      OM_ALL_COMPONENTS
   }

#undef OM_OBJECT

   ASSERT(false);
   return luabind::object();
}

void ScriptHost::LoadScript(std::string arg)
{
   if (luaL_dofile(L_, arg.c_str()) != 0) {
      OnError(lua_tostring(L_, -1));
   }
}


void ScriptHost::OnError(std::string description)
{
   stackDump(L_);
   LOG(WARNING) << "ScriptHost::OnError -> " << description;

   LOG(WARNING) << " -- start backtrace -- ";
   if (!last_error.empty()) {
      LOG(WARNING) << last_error;

      std::string item;
      std::stringstream ss(last_error_traceback);
      while(std::getline(ss, item)) {
         LOG(WARNING) << item;
      }
      last_error = "";
      last_error_traceback = "";
   }
   LOG(WARNING) << " -- end backtrace -- ";

#if 0
   luabind::object tb = luabind::globals(L_)["debug"]["traceback"];
   if (tb.is_valid()) { 
      std::string traceback = call_function<std::string>(tb);
      LOG(WARNING) << lua_tostring(L_, -1);
      LOG(WARNING) << "-- stack:";
      LOG(WARNING) << traceback;
      LOG(WARNING) << "-- endstack";
   }
#endif
#if 0
   int startTime = timeGetTime();
   while(timeGetTime() - startTime < 15000) {
      Sleep(0);
   }
#endif
}

luabind::object ScriptHost::LookupResource(std::string name)
{
   luabind::object result;
   std::shared_ptr<resources::Resource> obj = resources::ResourceManager2::GetInstance().LookupResource(name);
   return LuaResource::ConvertResourceToLua(L_, obj);
}

void ScriptHost::Call(std::string name, luabind::object arg1)
{
   try {
      luabind::object fn = globals(L_)[name];
      call_function<void>(fn, arg1);
   } CATCH_LUA_ERROR("calling function...");
}

om::EntityRef ScriptHost::CreateEntityRef(std::string kind)
{
   return CreateEntity(kind);
}

luabind::object ScriptHost::GetEntityRef(om::EntityId id)
{
   luabind::object result;
   auto i = entityMap_.find(id);
   if (i != entityMap_.end()) {
      result = luabind::object(L_, om::EntityRef(i->second));
   }
   return result;
}

om::GridPtr ScriptHost::CreateGrid()
{
   return Simulation::GetInstance().GetStore().AllocObject<om::Grid>();
}

om::EntityPtr ScriptHost::CreateEntity(std::string kind)
{
   dm::Store& store = Simulation::GetInstance().GetStore();
   om::EntityPtr entity = om::Stonehearth::CreateEntity(store, kind);
   entityMap_[entity->GetEntityId()] = entity;
   return entity;
}

void ScriptHost::DestroyEntity(std::weak_ptr<om::Entity> e)
{
   auto entity = e.lock();
   if (entity) {
      entityMap_.erase(entity->GetEntityId());
   }
}

std::shared_ptr<MultiPathFinder> ScriptHost::CreateMultiPathFinder(std::string name)
{
   return Simulation::GetInstance().CreateMultiPathFinder(name);
}

std::shared_ptr<FollowPath> ScriptHost::CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance)
{
   return Simulation::GetInstance().CreateFollowPath(entity, speed, path, close_to_distance);
}

std::shared_ptr<GotoLocation> ScriptHost::CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance)
{
   return Simulation::GetInstance().CreateGotoLocation(entity, speed, location, close_to_distance);
}

std::shared_ptr<GotoLocation> ScriptHost::CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance)
{
   return Simulation::GetInstance().CreateGotoEntity(entity, speed, target, close_to_distance);
}

std::shared_ptr<PathFinder> ScriptHost::CreatePathFinder(std::string name, om::EntityRef e)
{
   std::shared_ptr<PathFinder> result;
   auto entity = e.lock();
   if (entity) {
      result = Simulation::GetInstance().CreatePathFinder(name, entity);
   }
   return result;
}

void ScriptHost::Log(std::string str)
{
   LOG(WARNING) << str;
}

void GenerateTraceback(std::string reason)
{
   lua_State* L = ::radiant::simulation::ScriptHost::GetInstance().GetInterpreter();
   lua_pushstring(L, reason.c_str());
   lua_insert(L, 1);
   sh_pcall_callback_fun(L);
}

void ScriptHost::AssertFailed(std::string reason)
{
   lua_pushstring(L_, reason.c_str());
   lua_insert(L_, 1);
   stackDump(L_);
   sh_pcall_callback_fun(L_);
   OnError("Lua assert failed");
}

void ScriptHost::SendMsg(om::EntityRef entity, std::string msg)
{
   try {
      luabind::call_function<void>(game_["send_msg"], game_, entity, msg);
   } CATCH_LUA_ERROR("sending msg to entity");
}

void ScriptHost::SendMsg(om::EntityRef entity, std::string msg, const luabind::object& arg0)
{
   try {
      luabind::call_function<void>(game_["send_msg"], game_, entity, msg, arg0);
   } CATCH_LUA_ERROR("sending msg to entity");
}

void ScriptHost::Unstick(om::EntityRef e)
{
   // xxx - Unstick at start currently doesn't work.  When the terrain and the units are
   // created in the same loop, the physical layer has not yet had a chance to update the
   // collision shape for the group before this get called.  Ignore it for now.
#if 0
   auto entity = e.lock();
   if (entity) {
      Simulation::GetInstance().GetOctTree().Unstick(entity);
   }
#endif
}
