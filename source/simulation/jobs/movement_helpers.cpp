#include "pch.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

// This code may find paths that are illegal under the pathfinder.
// TODO: Reconcile this code with standard pathfinding rules.
bool MovementHelpers::GetStandableLocation(Simulation& sim, std::shared_ptr<om::Entity> const& entity, csg::Point3f& desiredLocation, csg::Point3f& actualLocation)
{
   phys::OctTree const& octTree = sim.GetOctTree();
   csg::Point3f candidate;
   
   candidate = desiredLocation;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      actualLocation = candidate;
      return true;
   }

   candidate = desiredLocation + csg::Point3f::unitY;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      actualLocation = candidate;
      return true;
   }

   candidate = desiredLocation - csg::Point3f::unitY;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      actualLocation = candidate;
      return true;
   }

   return false;
}
