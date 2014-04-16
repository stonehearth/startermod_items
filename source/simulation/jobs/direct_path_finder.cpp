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

#define DPF_LOG(level)   LOG(simulation.pathfinder.direct, level)

DirectPathFinder::DirectPathFinder(Simulation &sim, om::EntityRef entityRef, om::EntityRef targetRef) :
   sim_(sim),
   entityRef_(entityRef),
   targetRef_(targetRef),
   allowIncompletePath_(false),
   reversiblePath_(false)
{
   om::EntityPtr entity = entityRef_.lock();
   startLocation_ = entity->AddComponent<om::Mob>()->GetWorldGridLocation();
   logLevel_ = log_levels_.simulation.pathfinder.direct;
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetStartLocation(csg::Point3 const& startLocation)
{
   DPF_LOG(5) << "setting start location to " << std::boolalpha << startLocation;
   startLocation_ = startLocation;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetAllowIncompletePath(bool allowIncompletePath)
{
   DPF_LOG(5) << "setting allow incomplete path to " << std::boolalpha << allowIncompletePath;
   allowIncompletePath_ = allowIncompletePath;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetReversiblePath(bool reversiblePath)
{
   DPF_LOG(5) << "setting set reversible path to " << std::boolalpha << reversiblePath;
   reversiblePath_ = reversiblePath;
   return shared_from_this();
}

PathPtr DirectPathFinder::GetPath()
{
   om::EntityPtr entity = entityRef_.lock();
   om::EntityPtr target = targetRef_.lock();

   if (!entity || !target) {
      // Either the source or the destination is invalid.  Return nullptr.
      DPF_LOG(3) << "source or target entity is invalid.  returning nullptr.";
      return nullptr;
   }

   // Find a direct path to the point closest to the source from the current destination adjacent
   // region.  There may be other points in the destination for which a direct path exists.  This
   // method will fail to find those paths, but hey, you this is super fast.
   csg::Point3 const& start = startLocation_;
   csg::Point3 const targetLocation = target->AddComponent<om::Mob>()->GetWorldGridLocation();
   csg::Point3 end;

   bool haveEndPoint = MovementHelper(logLevel_).GetClosestPointAdjacentToEntity(sim_, start, target, end);

   if (!haveEndPoint) {
      // No point inside the target's adjacent region is actually standable.  There's *no way* to
      // get from here to there.  That's ok if we're just looking for a partial path, but we need
      // a point to run toward.  Just use the target's location for that.
      if (allowIncompletePath_) {
         DPF_LOG(5) << "could not find end point in target, using destination location";
         end = targetLocation;
      } else {
         // If there's no way to get there, there's just no way to get there.  Bail.
         DPF_LOG(5) << "could not find end point in target.  returning nullptr";
         return nullptr;
      }
   }

   // Walk the path from start to end and see how far we get.  
   std::vector<csg::Point3> points = MovementHelper(logLevel_).GetPathPoints(sim_, entity, reversiblePath_, start, end);

   // If we didn't reach the endpoint and we don't allow incomplete paths, bail.
   bool reachedEndPoint = !points.empty() && points.back() == end;
   if (!reachedEndPoint && !allowIncompletePath_) {
      DPF_LOG(5) << "could not find complete path to target.  returning nullptr";
      return nullptr;
   }

   // If we didn't get anywhere at all, just add the start point to the list to make sure
   // we always return a valid path
   if (points.empty()) {
      points.emplace_back(start);
   }

   // We have an acceptable path!  Compute the POI and return.
   csg::Point3 poi;
   om::DestinationPtr destinationPtr = target->GetComponent<om::Destination>();

   if (destinationPtr) {
      poi = destinationPtr->GetPointOfInterest(end - targetLocation);
      // transform poi to world coordinates
      poi += targetLocation;
   } else {
      poi = targetLocation;
   }

   PathPtr path = std::make_shared<Path>(points, entityRef_, targetRef_, poi);
   DPF_LOG(5) << "returning path: " << *path;
   return path;
}

std::ostream& simulation::operator<<(std::ostream& os, DirectPathFinder const& o)
{
   return os << "[DirectPathFinder ...]";
}
