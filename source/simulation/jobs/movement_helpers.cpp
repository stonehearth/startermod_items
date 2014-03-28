#include "pch.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

// returns true if this is a legal move (doesn't check adjacency yet though due to high game speed issues)
// toLocation contains the updated standing location when true and is unchanged when false
bool MovementHelpers::TestMoveXZ(Simulation& sim, std::shared_ptr<om::Entity> const& entity, csg::Point3f const& fromLocation, csg::Point3f& toLocation)
{
   phys::OctTree const& octTree = sim.GetOctTree();

   csg::Point3f toLocationProjected = toLocation;
   toLocationProjected.y = fromLocation.y;

   // should perform adjacency check on fromLocation and toLocationProjected, but can't yet at high game speeds

   csg::Point3f candidate = toLocationProjected;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      toLocation = candidate;
      return true;
   }

   candidate = toLocationProjected + csg::Point3f::unitY;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      toLocation = candidate;
      return true;
   }

   candidate = toLocationProjected - csg::Point3f::unitY;
   if (octTree.CanStandOn(entity, csg::ToClosestInt(candidate))) {
      toLocation = candidate;
      return true;
   }

   return false;
}
