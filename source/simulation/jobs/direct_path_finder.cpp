#include "pch.h"
#include "direct_path_finder.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

DirectPathFinder::DirectPathFinder(Simulation &sim, om::EntityRef entityRef, om::EntityRef targetRef) :
   sim_(sim),
   entityRef_(entityRef),
   targetRef_(targetRef),
   allowIncompletePath_(false)
{
   om::EntityPtr entity = entityRef_.lock();
   startLocation_ = entity->AddComponent<om::Mob>()->GetWorldGridLocation();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetStartLocation(csg::Point3 const& startLocation)
{
   startLocation_ = startLocation;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetAllowIncompletePath(bool allowIncompletePath)
{
   allowIncompletePath_ = allowIncompletePath;
   return shared_from_this();
}

PathPtr DirectPathFinder::GetPath()
{
   om::EntityPtr entity = entityRef_.lock();
   om::EntityPtr target = targetRef_.lock();

   csg::Point3 const& start = startLocation_;
   csg::Point3 end;
   bool haveEndPoint = MovementHelpers::GetClosestPointAdjacentToEntity(sim_, start, target, end);
   if (!haveEndPoint) {
      return nullptr;
   }

   std::vector<csg::Point3> points = MovementHelpers::GetPathPoints(sim_, entity, start, end);

   if (points.empty()) {
      return nullptr;
   }

   bool incompletePath = points.back() != end;

   if (incompletePath && !allowIncompletePath_) {
      return nullptr;
   }

   csg::Point3 poi;

   if (incompletePath) {
      poi = points.back();
   } else {
      csg::Point3 targetLocation = target->AddComponent<om::Mob>()->GetWorldGridLocation();
      om::DestinationPtr destinationPtr = target->GetComponent<om::Destination>();

      if (destinationPtr) {
         poi = destinationPtr->GetPointOfInterest(end - targetLocation);
         // transform poi to world coordinates
         poi += targetLocation;
      } else {
         poi = targetLocation;
      }
   }

   PathPtr path = std::make_shared<Path>(points, entityRef_, targetRef_, poi);

   return path;
}

std::ostream& simulation::operator<<(std::ostream& os, DirectPathFinder const& o)
{
   return os << "[DirectPathFinder ...]";
}
