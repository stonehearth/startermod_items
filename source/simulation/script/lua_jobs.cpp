#include "pch.h"
#include "lua_jobs.h"
#include "simulation/jobs/multi_path_finder.h"
#include "simulation/jobs/entity_destination.h"
#include "simulation/jobs/region_destination.h"
#include "simulation/jobs/follow_path.h"
#include "simulation/jobs/goto_location.h"
#include "csg/region.h"
#include "helpers.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;

std::vector<std::shared_ptr<Destination>> MultiPathFinderGetDestinations(const MultiPathFinder& mpf)
{
   std::vector<std::shared_ptr<Destination>> result;
   for (const auto& entry : mpf.GetDestinations()) {
      result.push_back(entry.second);
   }
   return result;
}

void PointListPushBack(PointList& l, math3d::ipoint3 pt) {
   l.push_back(pt);
}

math3d::ipoint3 PointListBack(const PointList& l)
{
   return l.back();
}

void LuaJobs::RegisterType(lua_State* L)
{
   module(L) [
      class_<Destination, std::shared_ptr<Destination> >("Destination")
         .def("set_enabled",     &Destination::SetEnabled)
         .def("is_enabled",      &Destination::IsEnabled)
      ,
      class_<EntityDestination, Destination, std::shared_ptr<Destination> >("EntityDestination")
         .def(constructor<om::EntityRef, const PointList&>())
         .def("get_entity",      &EntityDestination::GetEntity)
      ,
      class_<RegionDestination, Destination, std::shared_ptr<Destination> >("RegionDestination")
         .def(constructor<om::EntityRef, const om::RegionPtr>())
         .def("get_entity",      &RegionDestination::GetEntity)
      ,
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
         .def("get_destination",    &MultiPathFinder::GetDestination)
         .def("get_destinations",   &MultiPathFinderGetDestinations, return_stl_iterator)
         .def("get_active_destination_count", &MultiPathFinder::GetActiveDestinationCount)
         .def("remove_destination", &MultiPathFinder::RemoveDestination)
         .def("get_solution",       &MultiPathFinder::GetSolution)
         .def("set_enabled",        &MultiPathFinder::SetEnabled)
         .def("restart",            &MultiPathFinder::Restart)
      ,
      class_<PathFinder, std::shared_ptr<PathFinder> >("PathFinder")
         .def(tostring(self))
         .def("add_entity",         &MultiPathFinder::AddEntity)
         .def("add_destination",    &PathFinder::AddDestination)
         .def("remove_destination", &PathFinder::RemoveDestination)
         .def("get_solution",       &PathFinder::GetSolution)
      ,
      FollowPath::RegisterLuaType(L, "FollowPathJob"),
      GotoLocation::RegisterLuaType(L, "GotoLocation")
   ];
   module(L) [
      class_<PointList>("PointList")
         .def(constructor<>())
         .def("insert",             &PointListPushBack)
         .def("last",               &PointListBack)
   ];
}
