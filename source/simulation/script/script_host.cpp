#include "pch.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "radiant_file.h"
#include "script_host.h"
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


#define CATCH_LUA_ERROR(x) \
   catch (luabind::cast_failed& e) { \
      std::ostringstream out; \
      out << x << "(luabind::cast_failed: " << e.what() << " in call " << e.info().name() << ")"; \
      ScriptHost::GetInstance().OnError(out.str()); \
   } catch (luabind::error& e) { \
      std::ostringstream out; \
      out << x << "(luabind::error: " << e.what() << ")"; \
      ScriptHost::GetInstance().OnError(out.str()); \
   } catch (std::exception& e) { \
      std::ostringstream out; \
      out << x << "(std::exception: " << e.what() << ")"; \
      ScriptHost::GetInstance().OnError(out.str()); \
   }

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


ScriptHost::ScriptHost(lua_State* L) :
   L_(L)
{
   std::string game = configvm["game.script"].as<std::string>();

   ASSERT(!singleton_);
   singleton_ = this;

   // Load all the scripts...
   InitEnvironment();

   api_  = LoadScript("/radiant/server.lua");
   game_ctor_ = LoadScript(game);
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

void ScriptHost::InitEnvironment()
{
   int ii = lua_gettop(L_);

   set_pcall_callback(sh_pcall_callback_fun);

   module(L_) [
      class_<ScriptHost>("RadiantNative")
         .def("load_manifest",            &ScriptHost::LoadManifest)
         .def("load_json",                &ScriptHost::LoadJson)
         .def("load_animation",           &ScriptHost::LoadAnimation)
         .def("report_error",             &ScriptHost::ReportError)
         .def("create_entity",            &ScriptHost::CreateEntity)
         .def("get_entity",               &ScriptHost::GetEntity)
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
         .def("lua_require",              &ScriptHost::LuaRequire)
   ];
   lua_register(L_, "get_config_option", GetConfigOptions);

   LuaBasicTypes::RegisterType(L_);
   LuaObjectModel::RegisterType(L_);
   LuaJobs::RegisterType(L_);
   LuaNoise::RegisterType(L_);
   resources::Animation::RegisterType(L_);
   json::ConstJsonObject::RegisterLuaType(L_);

   module(L_) [
      csg::RegisterLuaType(L_),
      csg::VoronoiMap::RegisterLuaType(L_, "VoronoiMap"),
      csg::VoronoiMapSite::RegisterLuaType(L_, "VoronoiMapSite")
   ];

   //LuaWorkerScheduler::RegisterType(L_);

   globals(L_)["native"] = object(L_, this);
   //globals(L_)["require"] = globals(L_)["radiant_require"];

   //globals(L_)["package"]["path"] = "./scripts/?.lua;./scripts/lib/?.lua";
   //globals(L_)["package"]["cpath"] = "./scripts/?.lua;./scripts/clib/?.dll";

   globals(L_)["package"]["path"] = "./data/?.lua";
   globals(L_)["package"]["cpath"] = "";
}

void ScriptHost::CreateNew()
{
   // xxx : all c -> lua functions (except maybe update) should be on this clean callback thread.
   // this is to prevent state corruption and all sorts of other confusion which can result
   // from running on whatever state the main thread was in (e.g. if it most recently yielded
   // from a coroutine...)
   cb_thread_ = lua_newthread(L_);
   try {
      game_ = luabind::call_function<luabind::object>(game_ctor_);
      p1_ = globals(L_)["radiant"]["gamestate"]["_create_player"]('civ');
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
   try {
      currentGameTime = luabind::call_function<int>(api_["update"], interval);
   } CATCH_LUA_ERROR("update...");
}

void ScriptHost::CallGameHook(std::string const& stage)
{
   PROFILE_BLOCK();
   try {
      luabind::call_function<int>(api_["call_game_hook"], stage);
   } CATCH_LUA_ERROR((std::string("calling game hook: ") + stage).c_str());
}

void ScriptHost::Idle(platform::timer &timer)
{
   bool finished = false;
   while (!timer.expired() && !finished) {
      finished = (lua_gc(L_, LUA_GCSTEP, 1) != 0);
   }
}

class Reader {
public:
   Reader(std::ifstream& in) : contents(io::read_contents(in)), finished(false) { }

   static const char *Read(lua_State *L, void *ud, size_t *size) {
      return ((Reader *)ud)->ReadContents(size);
   }

private:
   const char *ReadContents(size_t *size) {
      if (finished) {
         contents.clear();
         return NULL;
      }
      finished = true;
      *size = contents.size();
      return contents.c_str();
   }

private:
   std::string contents;
   bool        finished;
};

luabind::object ScriptHost::LoadScript(std::string path)
{
   std::ifstream in;
   luabind::object obj;

   LOG(WARNING) << "loading script " << path;
#if 0
   // this is the awesome version that we'll use eventually, but we'll need
   // to ship our own decoda that knows how to load files from mods.
   if (!resources::ResourceManager2::GetInstance().OpenResource(uri, in)) {
      return obj;
   }

   Reader reader(in);
   if (lua_load(L_, &Reader::Read, &reader, uri.c_str()) != 0) {
      OnError(lua_tostring(L_, -1));
      return obj;
   }

   lua_pushstring(L_, uri.c_str());
   lua_pushcclosure(L_, LocalRequire, 1);
   lua_setglobal(L_, "require");

   if (lua_pcall(L_, 0, LUA_MULTRET, 0) != 0) {
      OnError(lua_tostring(L_, -1));
      return obj;
   }
#else
   // this is the slightly crappier version
   std::string filepath;
   try {
       filepath = resources::ResourceManager2::GetInstance().GetResourceFileName(path, ".lua");
   } catch (resources::Exception& e) {
      LOG(WARNING) << e.what();
	   return obj;
   }

   int error = luaL_loadfile(L_, filepath.c_str());
   if (error == LUA_ERRFILE) {
      OnError("Coud not open script file " + filepath);
   } else if (error != 0) {
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return obj;
   }
   if (lua_pcall(L_, 0, LUA_MULTRET, 0) != 0) {
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return obj;
   }

#endif
   obj = luabind::object(luabind::from_stack(L_, -1));
   lua_pop(L_, 1);
   return obj;
}

void ScriptHost::OnError(std::string description)
{
   LOG(WARNING) << "-- Lua Error Begin ------------------------------- ";
   std::string item;
   std::stringstream ss(description);
   while(std::getline(ss, item)) {
      LOG(WARNING) << "   " << item;
   }
   if (!last_error.empty()) {
      LOG(WARNING) << last_error;

      std::stringstream ss(last_error_traceback);
      while(std::getline(ss, item)) {
         LOG(WARNING) << "   " << item;
      }
      last_error = "";
      last_error_traceback = "";
   }
   LOG(WARNING) << "-- Lua Error End   ------------------------------- ";

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
#if 1
   int startTime = timeGetTime();
   while(timeGetTime() - startTime < 1500000) {
      Sleep(0);
      //DebugBreak();
   }
#endif
}

json::ConstJsonObject ScriptHost::LoadJson(std::string uri)
{
   return json::ConstJsonObject(resources::ResourceManager2::GetInstance().LookupJson(uri));
}

json::ConstJsonObject ScriptHost::LoadManifest(std::string uri)
{
   return json::ConstJsonObject(resources::ResourceManager2::GetInstance().LookupManifest(uri));
}

resources::AnimationPtr ScriptHost::LoadAnimation(std::string uri)
{
   return resources::ResourceManager2::GetInstance().LookupAnimation(uri);
}

luabind::object ScriptHost::LuaRequire(std::string uri)
{
   std::string path, name;
   std::ostringstream script;
   luabind::object obj;

   std::string canonical_path;
   try {
      canonical_path = resources::ResourceManager2::GetInstance().ConvertToCanonicalPath(uri, ".lua");
   } catch (resources::Exception& e) {
      LOG(WARNING) << e.what();
      return obj;
   }
   std::string key = canonical_path;

   auto i = required_.find(key);
   if (i != required_.end()) {
      obj = i->second;
   } else {
      obj = LoadScript(canonical_path);
      required_[key] = obj;
   }
   return obj;
}

void ScriptHost::Call(luabind::object fn, luabind::object arg1)
{
   try {
      luabind::object caller(cb_thread_, fn);
      call_function<void>(caller, arg1);
   } CATCH_LUA_ERROR("calling function...");
}

om::EntityRef ScriptHost::GetEntity(om::EntityId id)
{
   auto i = entityMap_.find(id);
   if (i != entityMap_.end()) {
      return i->second;
      result = luabind::object(L_, om::EntityRef(i->second));
   }
   return om::EntityRef();
}

om::GridPtr ScriptHost::CreateGrid()
{
   return Simulation::GetInstance().GetStore().AllocObject<om::Grid>();
}

om::EntityRef ScriptHost::CreateEntity()
{
   dm::Store& store = Simulation::GetInstance().GetStore();
   om::EntityPtr entity = store.AllocObject<om::Entity>();

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

std::shared_ptr<FollowPath> ScriptHost::CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object arrived_cb)
{
   luabind::object cb(cb_thread_, arrived_cb);
   std::shared_ptr<FollowPath> fp = std::make_shared<FollowPath>(entity, speed, path, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> ScriptHost::CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance, luabind::object arrived_cb)
{
   luabind::object cb(cb_thread_, arrived_cb);
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, location, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> ScriptHost::CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance, luabind::object arrived_cb)
{
   luabind::object cb(cb_thread_, arrived_cb);
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, target, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<PathFinder> ScriptHost::CreatePathFinder(std::string name, om::EntityRef e, luabind::object solved, luabind::object dst_filter)
{
   return Simulation::GetInstance().CreatePathFinder(name, e, solved, dst_filter);
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

std::string ScriptHost::PostCommand(luabind::object obj, std::string const& json)
{
   using namespace luabind;

   try {
      object coder = globals(L_)["radiant"]["json"];
      object data = call_function<object>(coder["decode"], json);
      object result = call_function<object>(obj, p1_, data);
      const char * ret = call_function<const char *>(coder["encode"], result);
	  LOG(WARNING) << "got back -> " << ret << " <-";
      return std::string(ret);
   } catch (std::exception &e) {
      return std::string("{'error': '") + e.what() + "'}";
   }
   // UNREACHABLE
   return "";
}

std::string ScriptHost::PostCommand(luabind::object fn, luabind::object self, std::string const& json)
{
   using namespace luabind;

   try {
      object coder = globals(L_)["radiant"]["json"];
      object data = call_function<object>(coder["decode"], json);
      object result = call_function<object>(fn, self, p1_, data);
      std::string ret = call_function<std::string>(coder["encode"], result);
      return ret;
   } catch (std::exception &e) {
      return std::string("{'error': '") + e.what() + "'}";
   }
   // UNREACHABLE
   return "";
}
