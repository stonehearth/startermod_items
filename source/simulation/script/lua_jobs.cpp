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

std::vector<om::DestinationRef> MultiPathFinderGetDestinations(const MultiPathFinder& mpf)
{
   std::vector<om::DestinationRef> result;
   for (const auto& entry : mpf.GetDestinations()) {
      auto dst = entry.second.lock();
      if (dst) {
         result.push_back(dst);
      }
   }
   return result;
}

void LuaJobs::RegisterType(lua_State* L)
{
   module(L) [
      class_<Path, std::shared_ptr<Path> >("Path")
         .def(tostring(self))
         .def("__towatch",          &DefaultToWatch<Path>)
         .def("get_destination",    &Path::GetDestination)
         .def("get_points",         &Path::GetPoints)
         .def("get_entity",         &Path::GetEntity)
      ,
      class_<MultiPathFinder, std::shared_ptr<MultiPathFinder> >("MultiPathFinder")
         .def(tostring(self))
         .def("add_entity",         &MultiPathFinder::AddEntity)
         .def("remove_entity",      &MultiPathFinder::RemoveEntity)
         .def("add_destination",    &MultiPathFinder::AddDestination)
         .def("get_destinations",   &MultiPathFinderGetDestinations, return_stl_iterator)
         .def("remove_destination", &MultiPathFinder::RemoveDestination)
         .def("get_solution",       &MultiPathFinder::GetSolution)
         .def("set_enabled",        &MultiPathFinder::SetEnabled)
         .def("restart",            &MultiPathFinder::Restart)
      ,
      class_<PathFinder, std::shared_ptr<PathFinder> >("PathFinder")
         .def(tostring(self))
         .def("add_destination",    &PathFinder::AddDestination)
         .def("remove_destination", &PathFinder::RemoveDestination)
         .def("get_solution",       &PathFinder::GetSolution)
      ,
      FollowPath::RegisterLuaType(L, "FollowPathJob"),
      GotoLocation::RegisterLuaType(L, "GotoLocation")
   ];
}
