#include "pch.h"
#include "bump_location.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

std::ostream& simulation::operator<<(std::ostream& os, BumpLocation const& o)
{
   return os << "[BumpLocation ...]";
}

BumpLocation::BumpLocation(Simulation& sim, om::EntityRef entity, csg::Point3f const& vector) :
   Task(sim, "bump location"),
   entity_(entity),
   vector_(vector.x, 0, vector.z)
{
}

bool BumpLocation::Work(platform::timer const& timer)
{
   auto entity = entity_.lock();
   if (!entity) {
      return false;
   }

   auto mob = entity->AddComponent<om::Mob>();

   csg::Point3f const currentLocation = mob->GetWorldLocation();
   csg::Point3 const currentGridLocation = csg::ToClosestInt(currentLocation);

   csg::Point3f const proposedLocation = currentLocation + vector_;
   csg::Point3 const proposedGridLocation = csg::ToClosestInt(proposedLocation);

   std::vector<csg::Point3> points;
   points = MovementHelpers::GetPathPoints(GetSim(), entity, true, currentGridLocation, proposedGridLocation);

   if (points.empty()) {
      // could perform a subgrid move
      return false;
   }

   csg::Point3 lastPoint = points.back();
   if (lastPoint == proposedGridLocation) {
      // move to the proposed floating point location
      mob->MoveTo(proposedLocation);
   } else {
      // proposed move was truncated, go as far as you can
      mob->MoveToGridAligned(lastPoint);
   }

   return false;
}
