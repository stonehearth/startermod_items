#include "../pch.h"
#include "open.h"
#include "lib/lua/script_host.h"
#include "simulation/simulation.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/bump_location.h"
#include "simulation/jobs/lua_job.h"
#include "simulation/jobs/path_finder.h"
#include "simulation/jobs/direct_path_finder.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/components/data_store.ridl.h"
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
   om::DataStorePtr datastore = Sim_AllocObject<om::DataStore>(L);
   datastore->SetData(newtable(L));

   return datastore;
}

luabind::object Sim_GetObject(lua_State* L, object id)
{
   dm::ObjectPtr obj;

   int id_type = type(id);
   if (id_type == LUA_TNUMBER) {
      dm::ObjectId object_id = object_cast<int>(id);
      obj = GetSim(L).GetStore().FetchObject<dm::Object>(object_id);
   } else if (id_type == LUA_TSTRING) {
      const char* addr = object_cast<const char*>(id);
      obj = GetSim(L).GetStore().FetchObject<dm::Object>(addr);
   }
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

std::shared_ptr<FollowPath> Sim_CreateFollowPath(lua_State *L, om::EntityRef entity, float speed, PathPtr path, float stop_distance, object arrived_cb)
{
   Simulation &sim = GetSim(L);
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<FollowPath> fp(new FollowPath(sim, entity, speed, path, stop_distance, cb));
   sim.AddTask(fp);
   return fp;
}

std::shared_ptr<BumpLocation> Sim_CreateBumpLocation(lua_State *L, om::EntityRef entity, csg::Point3f const& vector)
{
   Simulation &sim = GetSim(L);
   std::shared_ptr<BumpLocation> fp(new BumpLocation(sim, entity, vector));
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

std::shared_ptr<DirectPathFinder> Sim_CreateDirectPathFinder(lua_State *L, om::EntityRef entityRef, om::EntityRef targetRef)
{
   Simulation &sim = GetSim(L);
   return std::make_shared<DirectPathFinder>(sim, entityRef, targetRef);
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
DEFINE_INVALID_JSON_CONVERSION(DirectPathFinder);
DEFINE_INVALID_JSON_CONVERSION(BumpLocation);
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
            def("create_datastore",         &Sim_AllocDataStore),
            def("create_path_finder",       &Sim_CreatePathFinder),
            def("create_direct_path_finder",&Sim_CreateDirectPathFinder),
            def("create_follow_path",       &Sim_CreateFollowPath),
            def("create_bump_location",     &Sim_CreateBumpLocation),
            def("create_job",               &Sim_CreateJob),
            
            lua::RegisterTypePtr_NoTypeInfo<Path>("Path")
               .def("is_empty",           &Path::IsEmpty)
               .def("get_distance",       &Path::GetDistance)
               .def("get_points",         &Path::GetPoints, return_stl_iterator)
               .def("get_source",         &Path::GetSource)
               .def("get_destination",    &Path::GetDestination)
               .def("get_start_point",    &Path::GetStartPoint)
               .def("get_finish_point",   &Path::GetFinishPoint)
               .def("get_destination_point_of_interest",   &Path::GetDestinationPointOfInterest)
            ,
            lua::RegisterTypePtr_NoTypeInfo<PathFinder>("PathFinder")
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
            lua::RegisterType_NoTypeInfo<Simulation>("Simulation")
            ,
            lua::RegisterTypePtr_NoTypeInfo<DirectPathFinder>("DirectPathFinder")
               .def("set_start_location",        &DirectPathFinder::SetStartLocation)
               .def("set_allow_incomplete_path", &DirectPathFinder::SetAllowIncompletePath)
               .def("set_reversible_path",       &DirectPathFinder::SetReversiblePath)
               .def("get_path",                  &DirectPathFinder::GetPath)
            ,
            lua::RegisterTypePtr_NoTypeInfo<FollowPath>("FollowPath")
               .def("get_name", &FollowPath::GetName)
               .def("stop",     &FollowPath::Stop)
            ,
            lua::RegisterTypePtr_NoTypeInfo<BumpLocation>("BumpLocation")
            ,
            lua::RegisterTypePtr_NoTypeInfo<LuaJob>("LuaJob")

         ]
      ]
   ];
   globals(L)["_sim"] = object(L, sim);
}
