#include "../pch.h"
#include "open.h"
#include "lib/lua/script_host.h"
#include "simulation/simulation.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/goto_location.h"
#include "simulation/jobs/lua_job.h"
#include "simulation/jobs/path_finder.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/region.h"
#include "om/components/data_store.ridl.h"
#include "csg/util.h"
#include "lib/voxel/qubicle_brush.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace luabind;

std::ostream& operator<<(std::ostream& os, Simulation const&s)
{
   return (os << "[radiant simulation]");
}

static Simulation& GetSim(lua_State* L)
{
   Simulation* sim = object_cast<Simulation*>(globals(L)["_sim"]);
   if (!sim) {
      throw std::logic_error("could not find script host in interpreter");
   }
   return *sim;
}

template <typename T>
std::shared_ptr<T> Sim_AllocObject(lua_State* L)
{
   return GetSim(L).GetStore().AllocObject<T>();
}

om::EntityRef Sim_CreateEmptyEntity(lua_State* L)
{
   return Sim_AllocObject<om::Entity>(L);
}

om::EntityRef Sim_CreateEntity(lua_State* L, std::string const& uri)
{
   om::EntityPtr entity = Sim_AllocObject<om::Entity>(L);
   om::Stonehearth::InitEntity(entity, uri, L);
   return entity;
}

om::DataStorePtr Sim_AllocDataStore(lua_State* L)
{
   // make sure we return the strong pointer version
   om::DataStorePtr db = Sim_AllocObject<om::DataStore>(L);
   db->SetData(newtable(L));
   return db;
}


luabind::object Sim_GetObject(lua_State* L, object id)
{
   ASSERT(type(id) == LUA_TNUMBER);
   dm::ObjectId object_id = object_cast<int>(id);

   dm::Store& store = GetSim(L).GetStore();
   dm::ObjectPtr obj = store.FetchObject(object_id, -1);

   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   luabind::object lua_obj = host->CastObjectToLua(obj);
   return lua_obj;
}

void Sim_DestroyEntity(lua_State* L, std::weak_ptr<om::Entity> e)
{
   auto entity = e.lock();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      entity = nullptr;
      return GetSim(L).DestroyEntity(id);
   }
}

std::shared_ptr<FollowPath> Sim_CreateFollowPath(lua_State *L, om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance, object arrived_cb)
{
   Simulation &sim = GetSim(L);
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<FollowPath> fp(new FollowPath(sim, entity, speed, path, close_to_distance, cb));
   sim.AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Sim_CreateGotoLocation(lua_State *L, om::EntityRef entity, float speed, const csg::Point3& location, float close_to_distance, object arrived_cb)
{
   Simulation &sim = GetSim(L);
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<GotoLocation> fp(new GotoLocation(sim, entity, speed, csg::ToFloat(location), close_to_distance, cb));
   sim.AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Sim_CreateGotoEntity(lua_State *L, om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance, object arrived_cb)
{
   Simulation &sim = GetSim(L);
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<GotoLocation> fp(new GotoLocation(sim, entity, speed, target, close_to_distance, cb));
   sim.AddTask(fp);
   return fp;
}

std::shared_ptr<PathFinder> Sim_CreatePathFinder(lua_State *L, om::EntityRef s, std::string name)
{
   om::EntityPtr source = s.lock();
   if (source) {
      Simulation &sim = GetSim(L);
      std::shared_ptr<PathFinder> pf(PathFinder::Create(sim, name, source));
      sim.AddJobForEntity(source, pf);
      return pf;
   }
   return nullptr;
}

std::shared_ptr<LuaJob> Sim_CreateJob(lua_State *L, std::string const& name, object cb)
{
   Simulation &sim = GetSim(L);
   std::shared_ptr<LuaJob> job = std::make_shared<LuaJob>(sim, name, object(lua::ScriptHost::GetCallbackThread(L), cb));
   sim.AddJob(job);
   return job;
}

DEFINE_INVALID_JSON_CONVERSION(Path);
DEFINE_INVALID_JSON_CONVERSION(PathFinder);
DEFINE_INVALID_JSON_CONVERSION(FollowPath);
DEFINE_INVALID_JSON_CONVERSION(GotoLocation);
DEFINE_INVALID_JSON_CONVERSION(LuaJob);
DEFINE_INVALID_JSON_CONVERSION(Simulation);
DEFINE_INVALID_LUA_CONVERSION(Simulation)

void lua::sim::open(lua_State* L, Simulation* sim)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("sim") [
            def("create_empty_entity",      &Sim_CreateEmptyEntity),
            def("create_entity",            &Sim_CreateEntity),
            def("get_object",               &Sim_GetObject),
            def("destroy_entity",           &Sim_DestroyEntity),
            def("alloc_region",             &Sim_AllocObject<om::Region3Boxed>),
            def("alloc_region2",            &Sim_AllocObject<om::Region2Boxed>),
            def("create_data_store",        &Sim_AllocDataStore),
            def("create_path_finder",       &Sim_CreatePathFinder),
            def("create_follow_path",       &Sim_CreateFollowPath),
            def("create_goto_location",     &Sim_CreateGotoLocation),
            def("create_goto_entity",       &Sim_CreateGotoEntity),
            def("create_job",               &Sim_CreateJob),
            
            lua::RegisterTypePtr<Path>()
               .def("is_empty",           &Path::IsEmpty)
               .def("get_distance",       &Path::GetDistance)
               .def("get_points",         &Path::GetPoints, return_stl_iterator)
               .def("get_source",         &Path::GetSource)
               .def("get_destination",    &Path::GetDestination)
               .def("get_start_point",    &Path::GetStartPoint)
               .def("get_finish_point",   &Path::GetFinishPoint)
               .def("get_destination_point_of_interest",   &Path::GetDestinationPointOfInterest)
            ,
            lua::RegisterTypePtr<PathFinder>()
               .def("get_id",             &PathFinder::GetId)
               .def("set_source",         &PathFinder::SetSource)
               .def("add_destination",    &PathFinder::AddDestination)
               .def("remove_destination", &PathFinder::RemoveDestination)
               .def("set_solved_cb",      &PathFinder::SetSolvedCb)
               .def("set_search_exhausted_cb", &PathFinder::SetSearchExhaustedCb)
               .def("set_filter_fn",      &PathFinder::SetFilterFn)
               .def("get_solution",       &PathFinder::GetSolution)
               .def("set_debug_color",    &PathFinder::SetDebugColor)
               .def("is_idle",            &PathFinder::IsIdle)
               .def("stop",               &PathFinder::Stop)
               .def("start",              &PathFinder::Start)
               .def("restart",            &PathFinder::RestartSearch)
               .def("describe_progress",  &PathFinder::DescribeProgress)
            ,
            lua::RegisterType<Simulation>()
            ,
            lua::RegisterTypePtr<FollowPath>()
               .def("get_name", &FollowPath::GetName)
               .def("stop",     &FollowPath::Stop)
            ,
            lua::RegisterTypePtr<GotoLocation>()
               .def("stop",     &GotoLocation::Stop)
            ,
            lua::RegisterTypePtr<LuaJob>()

         ]
      ]
   ];
   globals(L)["_sim"] = object(L, sim);
}
