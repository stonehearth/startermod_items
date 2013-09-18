#include "../pch.h"
#include "open.h"
#include "lua/script_host.h"
#include "simulation/simulation.h"
#include "simulation/jobs/multi_path_finder.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/goto_location.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/region.h"

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

om::EntityRef Sim_CreateEmptyEntity()
{
   return Simulation::GetInstance().CreateEntity();
}

om::EntityRef Sim_CreateEntityByRef(lua_State* L, std::string const& entity_ref)
{
   om::EntityPtr entity = Simulation::GetInstance().CreateEntity();
   om::Stonehearth::InitEntityByRef(entity, entity_ref, L);
   return entity;
}

om::EntityRef Sim_CreateEntity(lua_State* L, std::string const& mod_name, std::string const& entity_name)
{
   om::EntityPtr entity = Simulation::GetInstance().CreateEntity();
   om::Stonehearth::InitEntity(entity, mod_name, entity_name, L);
   return entity;
}


void Sim_ExtendEntity(lua_State* L, om::EntityRef e, std::string const& mod_name, std::string const& entity_name)
{
   om::EntityPtr entity = e.lock();
   if (entity) {
      om::Stonehearth::InitEntity(entity, mod_name, entity_name, L);
   }
}


std::string Sim_GetEntityUri(lua_State* L, std::string const& mod_name, std::string const& entity_name)
{
   return res::ResourceManager2::GetInstance().GetEntityUri(mod_name, entity_name);
}

template <typename T>
std::shared_ptr<T> Sim_AllocObject()
{
   return Simulation::GetInstance().GetStore().AllocObject<T>();
}


om::EntityRef Sim_GetEntity(object id)
{
   using namespace luabind;
   if (type(id) == LUA_TNUMBER) {
      return Simulation::GetInstance().GetEntity(object_cast<int>(id));
   }
   if (type(id) == LUA_TSTRING) {
      dm::Store& store = Simulation::GetInstance().GetStore();
      dm::ObjectPtr obj = om::ObjectFormatter().GetObject(store, object_cast<std::string>(id));
      if (obj->GetObjectType() == om::EntityObjectType) {
         return std::static_pointer_cast<om::Entity>(obj);
      }
   }
   return om::EntityRef();
}


void Sim_DestroyEntity(std::weak_ptr<om::Entity> e)
{
   auto entity = e.lock();
   if (entity) {
      return Simulation::GetInstance().DestroyEntity(entity->GetObjectId());
   }
}

std::shared_ptr<FollowPath> Sim_CreateFollowPath(lua_State *L, om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance, object arrived_cb)
{
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<FollowPath> fp = std::make_shared<FollowPath>(entity, speed, path, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Sim_CreateGotoLocation(lua_State *L, om::EntityRef entity, float speed, const csg::Point3f& location, float close_to_distance, object arrived_cb)
{
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, location, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Sim_CreateGotoEntity(lua_State *L, om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance, object arrived_cb)
{
   object cb(lua::ScriptHost::GetCallbackThread(L), arrived_cb);
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, target, close_to_distance, cb);
   Simulation::GetInstance().AddTask(fp);
   return fp;
}

std::shared_ptr<MultiPathFinder> Sim_CreateMultiPathFinder(lua_State *L, std::string name)
{
   auto pf = std::make_shared<MultiPathFinder>(lua::ScriptHost::GetCallbackThread(L), name);
   Simulation::GetInstance().AddJob(pf);
   return pf;
}

std::shared_ptr<PathFinder> Sim_CreatePathFinder(lua_State *L, std::string name, om::EntityRef e, object solved_cb, object filter_cb)
{
   std::shared_ptr<PathFinder> pf;
   auto entity = e.lock();
   if (entity) {
      pf = std::make_shared<PathFinder>(lua::ScriptHost::GetCallbackThread(L), name, entity, solved_cb, filter_cb);
      Simulation::GetInstance().AddJob(pf);
   }
   return pf;
}

void lua::sim::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("sim") [
            def("xxx_extend_entity",        &Sim_ExtendEntity),
            def("xxx_get_entity_uri",       &Sim_GetEntityUri),
            def("create_entity",            &Sim_CreateEntity),
            def("create_empty_entity",      &Sim_CreateEmptyEntity),
            def("create_entity_by_ref",     &Sim_CreateEntityByRef),
            def("get_entity",               &Sim_GetEntity),
            def("destroy_entity",           &Sim_DestroyEntity),
            def("alloc_region",             &Sim_AllocObject<om::BoxedRegion3>),
            def("create_multi_path_finder", &Sim_CreateMultiPathFinder),
            def("create_path_finder",       &Sim_CreatePathFinder),
            def("create_follow_path",       &Sim_CreateFollowPath),
            def("create_goto_location",     &Sim_CreateGotoLocation),
            def("create_goto_entity",       &Sim_CreateGotoEntity),

            lua::RegisterTypePtr<Path>()
               .def("get_points",         &Path::GetPoints)
               .def("get_source",         &Path::GetSource)
               .def("get_destination",    &Path::GetDestination)
               .def("get_start_point",    &Path::GetStartPoint)
               .def("get_finish_point",   &Path::GetFinishPoint)
            ,
            lua::RegisterTypePtr<MultiPathFinder>()
               .def("add_entity",         &MultiPathFinder::AddEntity)
               .def("remove_entity",      &MultiPathFinder::RemoveEntity)
               .def("add_destination",    &MultiPathFinder::AddDestination)
               .def("remove_destination", &MultiPathFinder::RemoveDestination)
               .def("set_reverse_search", &MultiPathFinder::SetReverseSearch)
               .def("set_enabled",        &MultiPathFinder::SetEnabled)
               .def("is_idle",            &MultiPathFinder::IsIdle)
            ,
            lua::RegisterTypePtr<PathFinder>()
               .def("get_id",             &PathFinder::GetId)
               .def("add_destination",    &PathFinder::AddDestination)
               .def("remove_destination", &PathFinder::RemoveDestination)
               .def("set_solved_cb",      &PathFinder::SetSolvedCb)
               .def("set_filter_fn",      &PathFinder::SetFilterFn)
               .def("get_solution",       &PathFinder::GetSolution)
               .def("set_reverse_search", &PathFinder::SetReverseSearch)
               .def("is_idle",            &PathFinder::IsIdle)
               .def("to_weak_ref",        &ToWeakPathFinder)
               .def("stop",               &PathFinder::Stop)
               .def("start",              &PathFinder::Start)
            ,
            RegisterType<WeakObjectReference<PathFinder>>()
               .def("lock",               &WeakObjectReference<PathFinder>::Lock)
            ,
            lua::RegisterTypePtr<FollowPath>()
               .def("stop",     &FollowPath::Stop)
            ,
            lua::RegisterTypePtr<GotoLocation>()
               .def("stop",     &GotoLocation::Stop)
         ]
      ]
   ];
}
