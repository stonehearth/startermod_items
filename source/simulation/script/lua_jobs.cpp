#include "pch.h"
#include "lua_jobs.h"
#include "simulation/jobs/multi_path_finder.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/goto_location.h"
#include "csg/region.h"
#include "om/components/destination.h"
#include "helpers.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;

std::vector<om::EntityRef> MultiPathFinderGetDestinations(const MultiPathFinder& mpf)
{
   std::vector<om::EntityRef> result;
   for (const auto& entry : mpf.GetDestinations()) {
      auto dst = entry.second.lock();
      if (dst) {
         result.push_back(dst);
      }
   }
   return result;
}

template <class T>
class WeakObjectReference {
public:
   WeakObjectReference(std::shared_ptr<T> obj) : obj_(obj) {}

   std::shared_ptr<T> Lock() {
      return obj_.lock();
   }

private:
   std::weak_ptr<T>  obj_;
};

WeakObjectReference<PathFinder> ToWeakPathFinder(lua_State* L, std::shared_ptr<PathFinder> p)
{
   return WeakObjectReference<PathFinder>(p);
}


void LuaJobs::RegisterType(lua_State* L)
{
   module(L) [
      class_<Path, std::shared_ptr<Path> >("Path")
         .def(tostring(self))
         .def("__towatch",          &DefaultToWatch<Path>)
         .def("get_points",         &Path::GetPoints)
         .def("get_source",         &Path::GetSource)
         .def("get_destination",    &Path::GetDestination)
         .def("get_start_point",    &Path::GetStartPoint)
         .def("get_finish_point",   &Path::GetFinishPoint)
      ,
      class_<MultiPathFinder, std::shared_ptr<MultiPathFinder> >("MultiPathFinder")
         .def(tostring(self))
         .def("add_entity",         &MultiPathFinder::AddEntity)
         .def("remove_entity",      &MultiPathFinder::RemoveEntity)
         .def("add_destination",    &MultiPathFinder::AddDestination)
         .def("get_destinations",   &MultiPathFinderGetDestinations, return_stl_iterator)
         .def("remove_destination", &MultiPathFinder::RemoveDestination)
         .def("set_reverse_search", &MultiPathFinder::SetReverseSearch)
         .def("set_enabled",        &MultiPathFinder::SetEnabled)
         .def("is_idle",            &MultiPathFinder::IsIdle)
      ,
      class_<PathFinder, std::shared_ptr<PathFinder> >("PathFinder")
         .def(tostring(self))
         .def("get_id",             &PathFinder::GetId)
         .def("add_destination",    &PathFinder::AddDestination)
         .def("remove_destination", &PathFinder::RemoveDestination)
         .def("set_solved_cb",      &PathFinder::SetSolvedCb)
         .def("set_filter_fn",      &PathFinder::SetFilterFn)
         .def("get_solution",       &PathFinder::GetSolution)
         .def("set_reverse_search", &PathFinder::SetReverseSearch)
         .def("is_idle",            &PathFinder::IsIdle)
         .def("to_weak_ref",        &ToWeakPathFinder)
      ,
      class_<WeakObjectReference<PathFinder> >("WeakPathFinder")
         .def("lock",               &WeakObjectReference<PathFinder>::Lock)
      ,
      FollowPath::RegisterLuaType(L, "FollowPathJob"),
      GotoLocation::RegisterLuaType(L, "GotoLocation")
   ];
}
