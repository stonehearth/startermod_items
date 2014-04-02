#include "pch.h"
#include "bump_location.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define G_LOG(level)      LOG_CATEGORY(simulation.goto_location, level, GetName())

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

   // stepSize should be no more than 1.0 to prevent skipping a voxel in the navgrid test
   // unfortunately, entities can still wander through diagonal cracks
   float const stepSize = 1.0;
   csg::Point3f stepVector = csg::Point3f(vector_);
   stepVector.Normalize();
   stepVector.Scale(stepSize);

   auto mob = entity->GetComponent<om::Mob>();
   csg::Point3f currentLocation = mob->GetWorldLocation();
   csg::Point3f const destination = currentLocation + vector_;
   csg::Point3f nextLocation, resolvedLocation;

   float const distance = vector_.Length();
   float remainingDistance = distance;
   bool finished = false;

   while (!finished && remainingDistance > 0) {
      if (remainingDistance > stepSize) {
         nextLocation = currentLocation + stepVector;
         remainingDistance -= stepSize;
      } else {
         nextLocation = destination;
         remainingDistance = 0;
         finished = true;
      }

      bool passable = MovementHelpers::TestAdjacentMove(GetSim(), entity, true, currentLocation, nextLocation, resolvedLocation);

      if (passable) {
         currentLocation = resolvedLocation;
      } else {
         finished = true;
      }
   }

   mob->MoveTo(currentLocation);

   return true;
}
