#include "../pch.h"
#include "open.h"
#include "csg/util.h"
#include "lib/lua/script_host.h"
#include "simulation/simulation.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/bump_location.h"
#include "simulation/jobs/lua_job.h"
#include "simulation/jobs/bfs_path_finder.h"
#include "simulation/jobs/a_star_path_finder.h"
#include "simulation/jobs/direct_path_finder.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/components/data_store.ridl.h"
#include "om/components/mob.ridl.h"
#include "dm/store_save_state.h"
#include "dm/tracer_buffered.h"
#include "dm/lua_types.h"
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

DirectPathFinderPtr Sim_CreateDirectPathFinder(lua_State *L, om::EntityRef entityRef)
{
   Simulation &sim = GetSim(L);
   return std::make_shared<DirectPathFinder>(sim, entityRef);
}

std::shared_ptr<LuaJob> Sim_CreateJob(lua_State *L, std::string const& name, object cb)
{
   Simulation &sim = GetSim(L);
   std::shared_ptr<LuaJob> job = std::make_shared<LuaJob>(sim, name, object(lua::ScriptHost::GetCallbackThread(L), cb));
   sim.AddJob(job);
   return job;
}

std::shared_ptr<dm::StoreSaveState> Sim_CreateSaveState(lua_State *L)
{
   return std::make_shared<dm::StoreSaveState>();
}

void Sim_LoadObject(lua_State *L, std::shared_ptr<dm::StoreSaveState> s)
{
   Simulation &sim = GetSim(L);
   if (s) {
      s->Load(sim.GetStore());
   }
}

std::shared_ptr<dm::StoreSaveState> Sim_SaveObject(lua_State *L, dm::ObjectId id)
{
   Simulation &sim = GetSim(L);
   std::shared_ptr<dm::StoreSaveState> store = std::make_shared<dm::StoreSaveState>();
   store->Save(sim.GetStore(), id);
   return store;
}

struct TracerBufferedWrapper : public dm::TracerBuffered
{
   TracerBufferedWrapper(std::string const& name, dm::Store &store, dm::TraceCategories c) :
      dm::TracerBuffered(name, store),
      category(c)
   {
   }

   dm::TraceCategories category;
};

DECLARE_SHARED_POINTER_TYPES(TracerBufferedWrapper);

TracerBufferedWrapperPtr Sim_CreateTracer(lua_State* L, const char* name)
{
   static int c = (int)dm::USER_DEFINED_TRACES;
   dm::TraceCategories category = (dm::TraceCategories)(c++);

   Simulation &sim = GetSim(L);
   TracerBufferedWrapperPtr tracer = std::make_shared<TracerBufferedWrapper>(name, sim.GetStore(), category);
   sim.GetStore().AddTracer(tracer, category);
   return tracer;
}

AStarPathFinderPtr Sim_CreateAStarPathFinder(lua_State *L, om::EntityRef s, std::string const& name)
{
   AStarPathFinderPtr pf;
   om::EntityPtr source = s.lock();
   if (source) {
      Simulation &sim = GetSim(L);
      pf = AStarPathFinder::Create(sim, name, source);
      sim.AddJobForEntity(source, pf);
      return pf;
   }
   return pf;
}


BfsPathFinderPtr Sim_CreateBfsPathFinder(lua_State *L, om::EntityRef s, std::string const& name, int range)
{
   BfsPathFinderPtr pf;
   om::EntityPtr source = s.lock();
   if (source) {
      Simulation &sim = GetSim(L);
      pf = BfsPathFinder::Create(sim, source, name, range);
      sim.AddJobForEntity(source, pf);
      return pf;
   }
   return pf;
}


template <typename T>
std::shared_ptr<T> PathFinder_SetSolvedCb(lua_State* L, std::shared_ptr<T> pf, luabind::object unsafe_solved_cb)
{
   if (pf) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_solved_cb.interpreter());  
      luabind::object solved_cb = luabind::object(cb_thread, unsafe_solved_cb);
 
      pf->SetSolvedCb([solved_cb, cb_thread] (PathPtr path) mutable {
         try {
            solved_cb(luabind::object(cb_thread, path));
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
      });
   }
   return pf;
}

template <typename T>
std::shared_ptr<T> PathFinder_SetExhaustedCb(std::shared_ptr<T> pf, luabind::object unsafe_exhausted_cb)
{
   if (pf) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_exhausted_cb.interpreter());  
      luabind::object exhausted_cb = luabind::object(cb_thread, unsafe_exhausted_cb);

      pf->SetSearchExhaustedCb([exhausted_cb, cb_thread]() mutable {
         try {
            exhausted_cb();
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
      });
   }
   return pf;
}

BfsPathFinderPtr BfsPathFinder_SetFilterFn(BfsPathFinderPtr pf, luabind::object unsafe_filter_fn)
{
   if (pf) {
      BfsPathFinderRef p = pf;
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_filter_fn.interpreter());  
      luabind::object filter_fn = luabind::object(cb_thread, unsafe_filter_fn);

      pf->SetFilterFn([p, filter_fn, cb_thread](om::EntityPtr e) -> bool {
         try {
            if (LOG_IS_ENABLED(simulation.pathfinder.bfs, 5)) {
               BfsPathFinderPtr pathfinder = p.lock();
               if (pathfinder) {
                  pathfinder->Log(5, BUILD_STRING("calling filter function on " << *e));
               }
            }
            return luabind::call_function<bool>(filter_fn, om::EntityRef(e));
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
         return false;
      });
   }
   return pf;
}

PathPtr Path_CombinePaths(luabind::object const& table)
{
   ASSERT(type(table) == LUA_TTABLE);
   std::vector<PathPtr> paths;

   for (luabind::iterator i(table), end; i != end; i++) {
      paths.push_back(luabind::object_cast<PathPtr>(*i));
   }
   
   PathPtr combinedPath = CombinePaths(paths);
   return combinedPath;
}

DEFINE_INVALID_JSON_CONVERSION(Path);
DEFINE_INVALID_JSON_CONVERSION(PathFinder);
DEFINE_INVALID_JSON_CONVERSION(BfsPathFinder);
DEFINE_INVALID_JSON_CONVERSION(AStarPathFinder);
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
            lua::RegisterType_NoTypeInfo<Simulation>("Simulation")
            ,
            def("create_entity",             &Sim_CreateEntity),
            def("get_object",                &Sim_GetObject),
            def("destroy_entity",            &Sim_DestroyEntity),
            def("alloc_number_map",          &Sim_AllocObject<dm::NumberMap>),
            def("alloc_region",              &Sim_AllocObject<om::Region3Boxed>),
            def("alloc_region2",             &Sim_AllocObject<om::Region2Boxed>),
            def("create_datastore",          &Sim_AllocDataStore),
            def("create_astar_path_finder",  &Sim_CreateAStarPathFinder),
            def("create_bfs_path_finder",    &Sim_CreateBfsPathFinder),
            def("create_direct_path_finder", &Sim_CreateDirectPathFinder),
            def("create_follow_path",        &Sim_CreateFollowPath),
            def("create_bump_location",      &Sim_CreateBumpLocation),
            def("create_job",                &Sim_CreateJob),
            def("create_save_state",         &Sim_CreateSaveState),
            def("load_object",               &Sim_LoadObject),
            def("save_object",               &Sim_SaveObject),
            def("create_tracer",             &Sim_CreateTracer),
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
            lua::RegisterTypePtr_NoTypeInfo<AStarPathFinder>("AStarPathFinder")
               .def("get_id",             &AStarPathFinder::GetId)
               .def("set_source",         &AStarPathFinder::SetSource)
               .def("add_destination",    &AStarPathFinder::AddDestination)
               .def("remove_destination", &AStarPathFinder::RemoveDestination)
               .def("set_solved_cb",      &PathFinder_SetSolvedCb<AStarPathFinder>)
               .def("set_search_exhausted_cb", &PathFinder_SetExhaustedCb<AStarPathFinder>)
               .def("get_solution",       &AStarPathFinder::GetSolution)
               .def("set_debug_color",    &AStarPathFinder::SetDebugColor)
               .def("is_idle",            &AStarPathFinder::IsIdle)
               .def("stop",               &AStarPathFinder::Stop)
               .def("start",              &AStarPathFinder::Start)
               .def("restart",            &AStarPathFinder::RestartSearch)
               .def("describe_progress",  &AStarPathFinder::DescribeProgress)
            ,
            lua::RegisterTypePtr_NoTypeInfo<BfsPathFinder>("BfsPathFinder")
               .def("set_source",         &BfsPathFinder::SetSource)
               .def("set_solved_cb",      &PathFinder_SetSolvedCb<BfsPathFinder>)
               .def("set_search_exhausted_cb", &PathFinder_SetExhaustedCb<BfsPathFinder>)
               .def("set_filter_fn",      &BfsPathFinder_SetFilterFn)
               .def("reconsider_destination", &BfsPathFinder::ReconsiderDestination)
               .def("stop",               &BfsPathFinder::Stop)
               .def("start",              &BfsPathFinder::Start)
            ,
            lua::RegisterTypePtr_NoTypeInfo<DirectPathFinder>("DirectPathFinder")
               .def("set_start_location",        &DirectPathFinder::SetStartLocation)
               .def("set_end_location",          &DirectPathFinder::SetEndLocation)
               .def("set_destination_entity",    &DirectPathFinder::SetDestinationEntity)
               .def("set_allow_incomplete_path", &DirectPathFinder::SetAllowIncompletePath)
               .def("set_reversible_path",       &DirectPathFinder::SetReversiblePath)
               .def("get_path",                  &DirectPathFinder::GetPath)
            ,
            luabind::class_<TracerBufferedWrapper, std::shared_ptr<TracerBufferedWrapper>>("TracerBuffered")
               .def("flush",              &TracerBufferedWrapper::Flush)
               .def_readonly("category",  &TracerBufferedWrapper::category)
            ,
            lua::RegisterTypePtr_NoTypeInfo<FollowPath>("FollowPath")
               .def("get_name", &FollowPath::GetName)
               .def("stop",     &FollowPath::Stop)
            ,
            lua::RegisterTypePtr_NoTypeInfo<BumpLocation>("BumpLocation")
            ,
            lua::RegisterTypePtr_NoTypeInfo<LuaJob>("LuaJob")
            ,
            def("combine_paths", &Path_CombinePaths)
         ]
      ]
   ];
   globals(L)["_sim"] = object(L, sim);
}
