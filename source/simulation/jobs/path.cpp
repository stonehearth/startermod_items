#include "pch.h"
#include "path.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "movement_helpers.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

static int next_path_id_ = 1;

#define PF_LOG(level)   LOG(simulation.pathfinder.astar, level)

Path::Path(const std::vector<csg::Point3f>& points, om::EntityRef source, om::EntityRef destination, csg::Point3f const& poi) :
   points_(points),
   source_(source),
   destination_(destination),
   poi_(poi),
   id_(next_path_id_++)
{
   ASSERT(!points.empty());
}

std::vector<csg::Point3f> const& Path::GetPrunedPoints()
{
   if (prunedPoints_.size() == 0 && points_.size() > 0) {
      prunedPoints_ = MovementHelper().PruneCollinearPathPoints(points_);
   }
   return prunedPoints_;
}

std::ostream& Path::Format(std::ostream& os) const
{
   os << "[id:" << id_;
   om::EntityPtr source = source_.lock();
   if (source) {
      os << " " << *source << " -> ";
   }
   om::EntityPtr destination = destination_.lock();
   if (destination) {
      os << *destination << " -> ";
   }
   os << "endpoint: " << points_.back();
   os << " dpoi:" << poi_ << "]";
   return os;
}

float Path::GetDistance() const
{
   if (points_.size() > 1) {
      return points_.front().DistanceTo(points_.back());
   }
   return 0;
}

std::ostream& simulation::operator<<(std::ostream& os, const Path& in)
{
   return in.Format(os);
} 

csg::Point3f Path::GetStartPoint() const
{
   return points_.front();
}

csg::Point3f Path::GetFinishPoint() const
{
   return points_.back();
}

csg::Point3f Path::GetSourceLocation() const
{
   om::EntityPtr source = source_.lock();
   if (source) {
      om::EntityRef entityRoot;
      csg::Point3f location = source->GetComponent<om::Mob>()->GetWorldGridLocation(entityRoot);
      if (!om::IsRootEntity(entityRoot)) {
         PF_LOG(1) << source << " is not in the world";
      }
      return location;
   }
   return csg::Point3f::zero;
}

PathPtr radiant::simulation::CombinePaths(std::vector<PathPtr> const& paths)
{
   if (paths.empty()) {
      return nullptr;
   }

   PathPtr firstPath = paths.front();
   PathPtr lastPath = paths.back();
   std::vector<csg::Point3f> combinedPoints;

   for (PathPtr const& path : paths) {
      std::vector<csg::Point3f> const& points = path->GetPoints();

      if (!combinedPoints.empty()) {
         // really want to assert using OctTree::ValidMove(), but we don't have access to the simulation from here
         float distance = (points.front() - combinedPoints.back()).Length();
         if (distance >= 2) {
            PF_LOG(3) << "Combined paths are not adjacent. Distance between paths = " << distance <<
               ". This is an unusual or impossible move.";
         }
      }

      combinedPoints.insert(combinedPoints.end(), path->GetPoints().begin(), path->GetPoints().end());
   }

   PathPtr combinedPath = std::make_shared<Path>(
      combinedPoints, firstPath->GetSource(), lastPath->GetDestination(), lastPath->GetDestinationPointOfInterest()
   );
   return combinedPath;
}
