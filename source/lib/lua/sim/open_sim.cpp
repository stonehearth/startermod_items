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
#include "lib/voxel/qubicle_brush.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace luabind;

// xxx - why not just use std::weak_ptr<PathFinder> ???
template <class T>
class WeakObjectReference {
public:
   WeakObjectReference(std::shared_ptr<T> obj) : obj_(obj) {}

   std::shared_ptr<T> Lock() const {
      return obj_.lock();
   }

private:
   std::weak_ptr<T>  obj_;
};

std::ostream& operator<<(std::ostream& os, Simulation const&s)
{
   return (os << "[radiant simulation]");
}

std::ostream& operator<<(std::ostream& os, WeakObjectReference<PathFinder> const& o)
{
   auto p = o.Lock();
   if (!p) {
      return os << "[PathFinderRef expired]";
   }
   return os << "[PathFinderRef -> " << *p << " ]";
}

WeakObjectReference<PathFinder> ToWeakPathFinder(lua_State* L, std::shared_ptr<PathFinder> p)
{
   return WeakObjectReference<PathFinder>(p);
}

static Simulation& GetSim(lua_State* L)
{
   Simulation* sim = object_cast<Simulation*>(globals(L)["_sim"]);
   if (!sim) {
      throw std::logic_error("could not find script host in interpreter");
   }
   return *sim;
}

om::EntityRef Sim_CreateEmptyEntity(lua_State* L)
{
   return GetSim(L).CreateEntity();
}

om::EntityRef Sim_CreateEntity(lua_State* L, std::string const& uri)
{
   om::EntityPtr entity = GetSim(L).CreateEntity();
   om::Stonehearth::InitEntity(entity, uri, L);
   return entity;
}

std::string Sim_GetEntityUri(lua_State* L, std::string const& mod_name, std::string const& entity_name)
{
   return res::ResourceManager2::GetInstance().GetEntityUri(mod_name, entity_name);
}

template <typename T>
std::shared_ptr<T> Sim_AllocObject(lua_State* L)
{
   return GetSim(L).GetStore().AllocObject<T>();
}

om::DataStorePtr Sim_AllocDataStore(lua_State* L)
{
   // make sure we return the strong pointer version
   om::DataStorePtr db = Sim_AllocObject<om::DataStore>(L);
   db->SetData(newtable(L));
   return db;
}


om::EntityRef Sim_GetEntity(lua_State* L, object id)
{
   using namespace luabind;
   if (type(id) == LUA_TNUMBER) {
      return GetSim(L).GetEntity(object_cast<int>(id));
   }
   if (type(id) == LUA_TSTRING) {
      dm::Store& store = GetSim(L).GetStore();
      dm::ObjectPtr obj = om::ObjectFormatter().GetObject(store, object_cast<std::string>(id));
      if (obj->GetObjectType() == om::EntityObjectType) {
         return std::static_pointer_cast<om::Entity>(obj);
      }
   }
   return om::EntityRef();
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

std::shared_ptr<GotoLocation> Sim_CreateGotoLocation(lua_State *L, om::EntityRef entity, float speed, const csg::Point3f& location, float close_to_distance, object arrived_cb)
{
   Simulation &sim = GetSim(L);
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<GotoLocation> fp(new GotoLocation(sim, entity, speed, location, close_to_distance, cb));
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
      std::shared_ptr<PathFinder> pf(new PathFinder(sim, name, source));
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

std::shared_ptr<PathFinder> PathFinder_AddDestination(lua_State* L, std::shared_ptr<PathFinder> pf, om::EntityRef dst)
{
   pf->AddDestination(dst);
   return pf;
}

std::shared_ptr<PathFinder> PathFinder_SetSource(lua_State* L, std::shared_ptr<PathFinder> pf, csg::Point3 const& src)
{
   pf->SetSource(src);
   return pf;
}

std::shared_ptr<PathFinder> PathFinder_RemoveDestination(lua_State* L, std::shared_ptr<PathFinder> pf, dm::ObjectId dst)
{
   pf->RemoveDestination(dst);
   return pf;
}

std::shared_ptr<PathFinder> PathFinder_SetSolvedCb(lua_State* L, std::shared_ptr<PathFinder> pf, luabind::object cb)
{
   pf->SetSolvedCb(cb);
   return pf;
}

std::shared_ptr<PathFinder> PathFinder_SetFilterFn(lua_State* L, std::shared_ptr<PathFinder> pf, luabind::object cb)
{
   pf->SetFilterFn(cb);
   return pf;
}

DEFINE_INVALID_JSON_CONVERSION(Path);
DEFINE_INVALID_JSON_CONVERSION(PathFinder);
DEFINE_INVALID_JSON_CONVERSION(FollowPath);
DEFINE_INVALID_JSON_CONVERSION(GotoLocation);
DEFINE_INVALID_JSON_CONVERSION(WeakObjectReference<PathFinder>);
DEFINE_INVALID_JSON_CONVERSION(LuaJob);
DEFINE_INVALID_JSON_CONVERSION(Simulation);

void lua::sim::open(lua_State* L, Simulation* sim)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("sim") [
            def("create_empty_entity",      &Sim_CreateEmptyEntity),
            def("create_entity",            &Sim_CreateEntity),
            def("get_entity",               &Sim_GetEntity),
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
               .def("set_source",         &PathFinder_SetSource)
               .def("add_destination",    &PathFinder_AddDestination)
               .def("remove_destination", &PathFinder_RemoveDestination)
               .def("set_solved_cb",      &PathFinder_SetSolvedCb)
               .def("set_filter_fn",      &PathFinder_SetFilterFn)
               .def("get_solution",       &PathFinder::GetSolution)
               .def("set_debug_color",    &PathFinder::SetDebugColor)
               .def("is_idle",            &PathFinder::IsIdle)
               .def("to_weak_ref",        &ToWeakPathFinder)
               .def("stop",               &PathFinder::Stop)
               .def("start",              &PathFinder::Start)
               .def("restart",            &PathFinder::RestartSearch)
               .def("describe_progress",  &PathFinder::DescribeProgress)
            ,
            lua::RegisterType<Simulation>()
            ,
            lua::RegisterType<WeakObjectReference<PathFinder>>()
               .def("lock",               &WeakObjectReference<PathFinder>::Lock)
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
