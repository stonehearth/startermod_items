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
   startLocation_(csg::Point3::zero),
   endLocation_(csg::Point3::zero),
   useEntityForStartPoint_(true),
   useEntityForEndPoint_(true),
   allowIncompletePath_(false),
   reversiblePath_(false)
{
   logLevel_ = log_levels_.simulation.pathfinder.direct;
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetStartLocation(csg::Point3 const& startLocation)
{
   DPF_LOG(5) << "setting start location to " << startLocation;
   startLocation_ = startLocation;
   useEntityForStartPoint_ = false;
   return shared_from_this();
}

std::shared_ptr<DirectPathFinder> DirectPathFinder::SetEndLocation(csg::Point3 const& endLocation)
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

bool DirectPathFinder::GetEndPoints(csg::Point3& start, csg::Point3& end) const
{
   om::EntityPtr sourceEntity = entityRef_.lock();
   if (!sourceEntity) {
      return false;
   }

   if (useEntityForStartPoint_) {
      start = sourceEntity->AddComponent<om::Mob>()->GetWorldGridLocation();
   } else {
      start = startLocation_;
   }

   if (useEntityForEndPoint_) {
      om::EntityPtr destinationEntity = destinationRef_.lock();

      if (!destinationEntity) {
         DPF_LOG(3) << "destination entity is invalid.";
         return false;
      }

      // Find the point closest to the start in the destination entity's adjacent region.
      bool haveEndPoint = MovementHelper(logLevel_).GetClosestPointAdjacentToEntity(sim_, start, sourceEntity, destinationEntity, end);

      if (haveEndPoint) {
         return true;
      } else {
         // No point inside the destination entity's adjacent region is standable.  There is *no way* to
         // get from here to there.  That's ok if we're just looking for a partial path, but we need
         // a point to run toward.  Just use the destination entity's location for that.
         if (allowIncompletePath_) {
            DPF_LOG(5) << "could not find end point in destination entity, using destination location";
            end = destinationEntity->AddComponent<om::Mob>()->GetWorldGridLocation();
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

csg::Point3 DirectPathFinder::GetPointOfInterest(csg::Point3 const& end) const
{
   csg::Point3 poi = end;

   if (useEntityForEndPoint_) {
      om::EntityPtr destinationEntity = destinationRef_.lock();
      if (destinationEntity) {
         poi = MovementHelper().GetPointOfInterest(end, destinationEntity);
      }
   }

   return poi;
}

PathPtr DirectPathFinder::GetPath()
{
   csg::Point3 start, end;
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
   std::vector<csg::Point3> points = MovementHelper(logLevel_).GetPathPoints(sim_, entity, reversiblePath_, start, end);

   // If we didn't reach the endpoint and we don't allow incomplete paths, bail.
   bool reachedEndPoint = !points.empty() && points.back() == end;
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
   csg::Point3 poi = GetPointOfInterest(end);

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
