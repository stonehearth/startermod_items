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

DirectPathFinder::DirectPathFinder(Simulation &sim, om::EntityRef entityRef) :
   sim_(sim),
   entityRef_(entityRef),
   destinationRef_(),
   startLocation_(csg::Point3f::zero),
   endLocation_(csg::Point3f::zero),
   useEntityForStartPoint_(true),
   useEntityForEndPoint_(true),
   allowIncompletePath_(false),
   reversiblePath_(false)
{
   logLevel_ = log_levels_.simulation.pathfinder.direct;
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetStartLocation(csg::Point3f const& startLocation)
{
   DPF_LOG(5) << "setting start location to " << startLocation;
   startLocation_ = startLocation;
   useEntityForStartPoint_ = false;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetEndLocation(csg::Point3f const& endLocation)
{
   DPF_LOG(5) << "setting end location to " << endLocation;
   endLocation_ = endLocation;
   useEntityForEndPoint_ = false;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetDestinationEntity(om::EntityRef destinationRef)
{
   DPF_LOG(5) << "setting destination entity to (id) " << destinationRef.lock()->GetObjectId();
   destinationRef_ = destinationRef;
   useEntityForEndPoint_ = true;
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

bool DirectPathFinder::GetEndPoints(csg::Point3f& start, csg::Point3f& end) const
{
   om::EntityRef entityRoot;
   om::EntityPtr sourceEntity = entityRef_.lock();
   if (!sourceEntity) {
      DPF_LOG(3) << "source entity is invalid.";
      return false;
   }

   if (useEntityForStartPoint_) {
      start = sourceEntity->AddComponent<om::Mob>()->GetWorldGridLocation(entityRoot);
      if (!om::IsRootEntity(entityRoot)) {
         DPF_LOG(3) << "source entity is not in the world.";
         return false;
      }
   } else {
      start = startLocation_;
   }

   if (useEntityForEndPoint_) {
      om::EntityPtr destinationEntity = destinationRef_.lock();

      if (!destinationEntity) {
         DPF_LOG(3) << "destination entity is invalid.";
         return false;
      }

      csg::Point3f destinationLocation = destinationEntity->AddComponent<om::Mob>()->GetWorldGridLocation(entityRoot);
      if (!om::IsRootEntity(entityRoot)) {
         DPF_LOG(3) << "destination entity is not in the world.";
         return false;
      }

      // Find the point closest to the start in the destination entity's adjacent region. Assign that point to end.
      bool haveEndPoint = MovementHelper(logLevel_).GetClosestPointAdjacentToEntity(sim_, start, sourceEntity, destinationEntity, end);

      if (haveEndPoint) {
         return true;
      } else {
         // No point inside the destination entity's adjacent region is standable.  There is *no way* to
         // get from here to there.  That's ok if we're just looking for a partial path, but we need
         // a point to run toward.  Just use the destination entity's location for that.
         if (allowIncompletePath_) {
            DPF_LOG(5) << "could not find end point in destination entity, using destination location";
            end = destinationLocation;
            return true;
         } else {
            // If there's no way to get there, there's just no way to get there.  Bail.
            DPF_LOG(5) << "could not find end point in destination.";
            return false;
         }
      }
   } else {
      // Using a Point destination
      end = endLocation_;
      return true;
   }
}

bool DirectPathFinder::GetPointOfInterest(csg::Point3f const& end, csg::Point3f& poi) const
{
   if (useEntityForEndPoint_) {
      om::EntityPtr destinationEntity = destinationRef_.lock();
      if (destinationEntity) {
         return MovementHelper(logLevel_).GetPointOfInterest(destinationEntity, end, poi);
      } else {
         return false;
      }
   } else {
      // we're just pathing to a point, so return that point
      poi = end;
      return true;
   }
}

PathPtr DirectPathFinder::GetPath()
{
   csg::Point3f start, end;
   om::EntityPtr entity = entityRef_.lock();

   if (!entity) {
      DPF_LOG(3) << "entity is invalid. returning nullptr.";
      return nullptr;
   }

   bool success = GetEndPoints(start, end);
   if (!success) {
      DPF_LOG(5) << "unable to get end points. returning nullptr";
      return nullptr;
   }

   // Walk the path from start to end and see how far we get.
   // GetPathPoints should always return at least the starting point, but check if empty in case this ever changes.
   std::vector<csg::Point3f> points;
   bool reachedEndPoint = MovementHelper(logLevel_).GetPathPoints(sim_, entity, reversiblePath_, start, end, points);

   // If we didn't reach the endpoint and we don't allow incomplete paths, bail.
   if (!reachedEndPoint && !allowIncompletePath_) {
      DPF_LOG(5) << "could not find complete path to destination. returning nullptr";
      return nullptr;
   }

   // If we didn't get anywhere at all, just add the start point to the list to make sure
   // we always return a valid path
   if (points.empty()) {
      points.emplace_back(start);
   }

   // We have an acceptable path!  Compute the POI.
   csg::Point3f poi;
   bool isValidPoi = GetPointOfInterest(end, poi);
   if (!isValidPoi) {
      DPF_LOG(1) << "failed to get point of interest";
      return nullptr;
   }

   // Create the path object and return
   // destinationRef will be invalid if using a point as the destination
   PathPtr path = std::make_shared<Path>(points, entityRef_, destinationRef_, poi);
   DPF_LOG(5) << "returning path: " << *path;

   return path;
}

std::ostream& simulation::operator<<(std::ostream& os, DirectPathFinder const& o)
{
   return os << "[DirectPathFinder ...]";
}
