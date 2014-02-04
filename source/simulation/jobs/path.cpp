#include "pch.h"
#include "path.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

static int next_path_id_ = 1;

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder, level, "path " << id_)

Path::Path(const std::vector<csg::Point3>& points, om::EntityRef source, om::EntityRef destination, csg::Point3 const& poi) :
   points_(points),
   source_(source),
   destination_(destination),
   poi_(poi),
   id_(next_path_id_++)
{
   ASSERT(!points.empty());
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

csg::Point3 Path::GetStartPoint() const
{
   return points_.front();
}

csg::Point3 Path::GetFinishPoint() const
{
   return points_.back();
}

csg::Point3 Path::GetSourceLocation() const
{
   om::EntityPtr source = source_.lock();
   if (source) {
      return source->GetComponent<om::Mob>()->GetWorldGridLocation();
   }
   return csg::Point3(0, 0, 0);
}
