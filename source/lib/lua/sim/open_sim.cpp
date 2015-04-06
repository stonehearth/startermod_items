#include "../pch.h"
#include "open.h"
#include "build_number.h"
#include "csg/util.h"
#include "lib/lua/script_host.h"
#include "simulation/simulation.h"
#include "simulation/jobs/lua_job.h"
#include "simulation/jobs/bfs_path_finder.h"
#include "simulation/jobs/a_star_path_finder.h"
#include "simulation/jobs/direct_path_finder.h"
#include "simulation/jobs/filter_result_cache.h"
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

IMPLEMENT_TRIVIAL_TOSTRING(FilterResultCache);

std::ostream& operator<<(std::ostream& os, Simulation const&s)
{
   return (os << "[radiant simulation]");
}

std::string Sim_GetVersion(lua_State* L)
{
   return PRODUCT_FILE_VERSION_STR;
}

template <typename T>
std::shared_ptr<T> Sim_AllocObject(lua_State* L)
{
   return Simulation::GetInstance().GetStore().AllocObject<T>();
}

om::EntityRef Sim_CreateEntity(lua_State* L, const char* uri)
{
   om::EntityPtr entity = Sim_AllocObject<om::Entity>(L);
   om::Stonehearth::InitEntity(entity, uri, L);
   return entity;
}

luabind::object Sim_GetObject(lua_State* L, object id)
{
   dm::ObjectPtr obj;

   int id_type = type(id);
   if (id_type == LUA_TNUMBER) {
      dm::ObjectId object_id = object_cast<int>(id);
      obj = Simulation::GetInstance().GetStore().FetchObject<dm::Object>(object_id);
   } else if (id_type == LUA_TSTRING) {
      const char* addr = object_cast<const char*>(id);
      obj = Simulation::GetInstance().GetStore().FetchObject<dm::Object>(addr);
   }
   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   luabind::object lua_obj = host->CastObjectToLua(obj);
   return lua_obj;
}

om::DataStoreRef Sim_AllocDataStore(lua_State* L)
{
   // Return the weak ptr version.
   om::DataStoreRef datastore = Simulation::GetInstance().AllocDatastore();
   datastore.lock()->SetData(newtable(L));

   return datastore;
}

void Sim_DestroyEntity(lua_State* L, std::weak_ptr<om::Entity> e)
{
   auto entity = e.lock();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      entity = nullptr;
      Simulation::GetInstance().DestroyEntity(id);
   }
}

DirectPathFinderPtr Sim_CreateDirectPathFinder(lua_State *L, om::EntityRef entityRef)
{
   return std::make_shared<DirectPathFinder>(entityRef);
}

std::shared_ptr<LuaJob> Sim_CreateJob(lua_State *L, std::string const& name, object cb)
{
   Simulation &sim = Simulation::GetInstance();
   std::shared_ptr<LuaJob> job = std::make_shared<LuaJob>(name, object(lua::ScriptHost::GetCallbackThread(L), cb));
   sim.AddJob(job);
   return job;
}

std::shared_ptr<dm::StoreSaveState> Sim_CreateSaveState(lua_State *L)
{
   return std::make_shared<dm::StoreSaveState>();
}

void Sim_LoadObject(lua_State *L, std::shared_ptr<dm::StoreSaveState> s)
{
   Simulation &sim = Simulation::GetInstance();
   if (s) {
      s->Load(sim.GetStore());
   }
}

std::shared_ptr<dm::StoreSaveState> Sim_SaveObject(lua_State *L, dm::ObjectId id)
{
   Simulation &sim = Simulation::GetInstance();
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

   Simulation &sim = Simulation::GetInstance();
   TracerBufferedWrapperPtr tracer = std::make_shared<TracerBufferedWrapper>(name, sim.GetStore(), category);
   sim.GetStore().AddTracer(tracer, category);
   return tracer;
}

float Sim_GetBaseWalkSpeed(lua_State* L)
{
   return Simulation::GetInstance().GetBaseWalkSpeed();
}

bool Sim_IsValidMove(lua_State *L, om::EntityRef entityRef, bool reversible, csg::Point3f const& from, csg::Point3f const& to)
{
   om::EntityPtr entity = entityRef.lock();
   if (entity) {
      Simulation &sim = Simulation::GetInstance();
      phys::OctTree& octTree = sim.GetOctTree();
      bool valid = octTree.ValidMove(entity, reversible, csg::ToClosestInt(from), csg::ToClosestInt(to));
      return valid;
   } else {
      return false;
   }
}

AStarPathFinderPtr Sim_CreateAStarPathFinder(lua_State *L, om::EntityRef s, std::string const& name)
{
   AStarPathFinderPtr pf;
   om::EntityPtr source = s.lock();
   if (source) {
      Simulation &sim = Simulation::GetInstance();
      pf = AStarPathFinder::Create(name, source);
      sim.AddJobForEntity(source, pf);
      return pf;
   }
   return pf;
}


BfsPathFinderPtr Sim_CreateBfsPathFinder(lua_State *L, om::EntityRef s, std::string const& name, int range)
{
   Simulation &sim = Simulation::GetInstance();
   BfsPathFinderPtr pf;
   om::EntityPtr source = s.lock();
   if (source) {
      pf = BfsPathFinder::Create(source, name, range);
      sim.AddJobForEntity(source, pf);
      return pf;
   }
   return pf;
}

template <typename T>
void Pathfinder_Destroy(lua_State* L, std::shared_ptr<T> pf)
{
   if (pf) {
      Simulation &sim = Simulation::GetInstance();
      pf->Stop();
      Simulation::GetInstance().RemoveJobForEntity(pf->GetEntity().lock(), pf); 
   }
}

template <typename T>
std::shared_ptr<T> PathFinder_SetSolvedCb(lua_State* L, std::shared_ptr<T> pf, luabind::object unsafe_solved_cb)
{
   if (pf) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_solved_cb.interpreter());  
      luabind::object solved_cb = luabind::object(cb_thread, unsafe_solved_cb);
 
      pf->SetSolvedCb([solved_cb, cb_thread] (PathPtr path) mutable {
         MEASURE_TASK_TIME(Simulation::GetInstance().GetOverviewPerfTimeline(), "lua cb");
         try {
            luabind::object result = solved_cb(luabind::object(cb_thread, path));
            if (luabind::type(result) == LUA_TBOOLEAN) {
               return luabind::object_cast<bool>(result);
            }
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
         return true;
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
         MEASURE_TASK_TIME(Simulation::GetInstance().GetOverviewPerfTimeline(), "lua cb");
         try {
            exhausted_cb();
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
      });
   }
   return pf;
}

FilterResultCachePtr FilterResultCache_SetFilterFn(FilterResultCachePtr frc, luabind::object unsafe_filter_fn)
{
   if (frc) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_filter_fn.interpreter());  
      luabind::object filter_fn = luabind::object(cb_thread, unsafe_filter_fn);

      frc->SetFilterFn([filter_fn, cb_thread](om::EntityPtr e) mutable -> bool {
         MEASURE_TASK_TIME(Simulation::GetInstance().GetOverviewPerfTimeline(), "lua cb");
         try {
            LOG_CATEGORY(simulation.pathfinder.bfs, 5, "calling filter function on " << *e);
            luabind::object result = filter_fn(om::EntityRef(e));
            if (luabind::type(result) == LUA_TNIL) {
               return false;
            }
            if (luabind::type(result) == LUA_TBOOLEAN) {
               return luabind::object_cast<bool>(result);
            }
            return true;   // not nil or false is good enough for me!
         } catch (std::exception const& e) {
            lua::ScriptHost::ReportCStackException(cb_thread, e);
         }
         return false;
      });
   }
   return frc;
}

template <typename T>
luabind::object CppToLuaArray(lua_State *L, std::vector<T> const& vector)
{
   luabind::object array = luabind::newtable(L);
   int i = 1;

   for (T const& element : vector) {
      array[i++] = luabind::object(L, element);
   }

   return array;
}

luabind::object Path_GetPoints(lua_State *L, PathPtr const& path)
{
   return CppToLuaArray(L, path->GetPoints());
}

luabind::object Path_GetPrunedPoints(lua_State *L, PathPtr const& path)
{
   return CppToLuaArray(L, path->GetPrunedPoints());
}

PathPtr Path_CombinePaths(luabind::object const& table)
{
   ASSERT(type(table) == LUA_TTABLE);
   std::vector<PathPtr> paths;

   for (luabind::iterator i(table), end; i != end; ++i) {
      paths.push_back(luabind::object_cast<PathPtr>(*i));
   }
   
   PathPtr combinedPath = Path::CombinePaths(paths);
   return combinedPath;
}

PathPtr Path_CreatePathSubset(PathPtr path, int startIndex, int finishIndex)
{
   // convert indices from base 1 to base 0
   return Path::CreatePathSubset(path, startIndex-1, finishIndex-1);
}

DEFINE_INVALID_JSON_CONVERSION(Path);
DEFINE_INVALID_JSON_CONVERSION(PathFinder);
DEFINE_INVALID_JSON_CONVERSION(BfsPathFinder);
DEFINE_INVALID_JSON_CONVERSION(AStarPathFinder);
DEFINE_INVALID_JSON_CONVERSION(FollowPath);
DEFINE_INVALID_JSON_CONVERSION(DirectPathFinder);
DEFINE_INVALID_JSON_CONVERSION(LuaJob);
DEFINE_INVALID_JSON_CONVERSION(Simulation);

void lua::sim::open(lua_State* L, Simulation* sim)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("sim") [
            lua::RegisterType_NoTypeInfo<Simulation>("Simulation")
            ,
            def("get_version",               &Sim_GetVersion),
            def("create_entity",             &Sim_CreateEntity),
            def("get_object",                &Sim_GetObject),
            def("destroy_entity",            &Sim_DestroyEntity),
            def("alloc_number_map",          &Sim_AllocObject<dm::NumberMap>),
            def("alloc_region3",             &Sim_AllocObject<om::Region3fBoxed>),
            def("alloc_region2",             &Sim_AllocObject<om::Region2fBoxed>),
            def("create_datastore",          &Sim_AllocDataStore),
            def("create_astar_path_finder",  &Sim_CreateAStarPathFinder),
            def("create_bfs_path_finder",    &Sim_CreateBfsPathFinder),
            def("create_direct_path_finder", &Sim_CreateDirectPathFinder),
            def("create_job",                &Sim_CreateJob),
            def("create_save_state",         &Sim_CreateSaveState),
            def("load_object",               &Sim_LoadObject),
            def("save_object",               &Sim_SaveObject),
            def("create_tracer",             &Sim_CreateTracer),
            def("get_base_walk_speed",       &Sim_GetBaseWalkSpeed),
            def("is_valid_move",             &Sim_IsValidMove),
            lua::RegisterTypePtr_NoTypeInfo<Path>("Path")
               .def("is_empty",           &Path::IsEmpty)
               .def("get_distance",       &Path::GetDistance)
               .def("get_points",         &Path_GetPoints)
               .def("get_pruned_points",  &Path_GetPrunedPoints)
               .def("get_source",         &Path::GetSource)
               .def("get_destination",    &Path::GetDestination)
               .def("get_start_point",    &Path::GetStartPoint)
               .def("get_finish_point",   &Path::GetFinishPoint)
               .def("get_destination_point_of_interest",   &Path::GetDestinationPointOfInterest)
            ,
            lua::RegisterTypePtr_NoTypeInfo<AStarPathFinder>("AStarPathFinder")
               .def("get_id",             &AStarPathFinder::GetId)
               .def("get_name",           &AStarPathFinder::GetName)
               .def("set_source",         &AStarPathFinder::SetSource)
               .def("add_destination",    &AStarPathFinder::AddDestination)
               .def("remove_destination", &AStarPathFinder::RemoveDestination)
               .def("set_max_steps",      &AStarPathFinder::SetMaxSteps)
               .def("set_solved_cb",      &PathFinder_SetSolvedCb<AStarPathFinder>)
               .def("set_search_exhausted_cb", &PathFinder_SetExhaustedCb<AStarPathFinder>)
               .def("get_solution",       &AStarPathFinder::GetSolution)
               .def("set_debug_color",    &AStarPathFinder::SetDebugColor)
               .def("is_idle",            &AStarPathFinder::IsIdle)
               .def("stop",               &AStarPathFinder::Stop)
               .def("start",              &AStarPathFinder::Start)
               .def("destroy",            &Pathfinder_Destroy<AStarPathFinder>)
               .def("restart",            &AStarPathFinder::RestartSearch)
            ,
            lua::RegisterTypePtr_NoTypeInfo<BfsPathFinder>("BfsPathFinder")
               .def("get_id",             &BfsPathFinder::GetId)
               .def("get_name",           &BfsPathFinder::GetName)
               .def("set_source",         &BfsPathFinder::SetSource)
               .def("set_solved_cb",      &PathFinder_SetSolvedCb<BfsPathFinder>)
               .def("set_search_exhausted_cb", &PathFinder_SetExhaustedCb<BfsPathFinder>)
               .def("set_filter_result_cache",  &BfsPathFinder::SetFilterResultCache)
               .def("reconsider_destination", &BfsPathFinder::ReconsiderDestination)
               .def("stop",               &BfsPathFinder::Stop)
               .def("start",              &BfsPathFinder::Start)
               .def("destroy",            &Pathfinder_Destroy<BfsPathFinder>)
            ,
            lua::RegisterTypePtr_NoTypeInfo<FilterResultCache>("FilterResultCache")
               .def(constructor<>())
               .def("clear_cache_entry",  &FilterResultCache::ClearCacheEntry)     
               .def("set_filter_fn",      &FilterResultCache_SetFilterFn)
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
               .def("start",              &TracerBufferedWrapper::Start)
               .def("stop",               &TracerBufferedWrapper::Stop)
               .def_readonly("category",  &TracerBufferedWrapper::category)
            ,
            lua::RegisterTypePtr_NoTypeInfo<LuaJob>("LuaJob")
            ,
            def("combine_paths", &Path_CombinePaths),
            def("create_path_subset", &Path_CreatePathSubset)
         ]
      ]
   ];
   globals(L)["_sim"] = object(L, sim);
}
