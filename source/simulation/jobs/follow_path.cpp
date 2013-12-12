#include "pch.h"
#include "follow_path.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"
#include "csg/util.h" // xxx: should be csg/csg.h

using namespace ::radiant;
using namespace ::radiant::simulation;

#define FP_LOG(level)      LOG_CATEGORY(simulation.follow_path, level, GetName())

std::ostream& simulation::operator<<(std::ostream& os, FollowPath const& o)
{
   return os << "[FollowPath ...]";
}

FollowPath::FollowPath(Simulation& sim, om::EntityRef e, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object arrived_cb) :
   Task(sim, "follow path"),
   entity_(e),
   path_(path),
   pursuing_(0),
   speed_(speed),
   close_to_distance_(close_to_distance),
   arrived_cb_(arrived_cb)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      mob->SetMoving(false);
   }
   Report("constructor");
}

static float angle(const csg::Point3f &v)
{
   return (float)(atan2(v.z, -v.x) - atan2(-1, 0));
}


bool FollowPath::Work(const platform::timer &timer)
{
   Report("running");

   auto entity = entity_.lock();
   if (!entity) {
      return false;
   }

   float maxDistance = speed_ * GetSim().GetBaseWalkSpeed();
   auto mob = entity->GetComponent<om::Mob>();
   const std::vector<csg::Point3> &points = path_->GetPoints();

   while (!Arrived(mob) && !Obstructed() && maxDistance > 0)  {
      const csg::Point3f &current = mob->GetLocation();
      const csg::Point3f &goal = csg::ToFloat(points[pursuing_]);

      csg::Point3f direction = csg::Point3f(goal - current);
      float distance = direction.Length();
      if (distance < maxDistance) {
         mob->MoveTo(csg::ToFloat(points[pursuing_]));
         maxDistance -= distance;
         pursuing_++;
      } else {
         mob->MoveTo(current + (direction * (maxDistance / distance)));
         maxDistance = 0;
      }
      mob->TurnTo(angle(direction) * 180 / csg::k_pi);
   }
   if (Arrived(mob)) {
      Report("arrived!");
      if (luabind::type(arrived_cb_) == LUA_TFUNCTION) {
         luabind::call_function<void>(arrived_cb_);
      }
      return false;
   }
   return true;
}

bool FollowPath::Arrived(om::MobPtr mob)
{
   ASSERT(path_);
   if (close_to_distance_) {
      ASSERT(false);
   }
   return (unsigned int)pursuing_ >= path_->GetPoints().size();
}

bool FollowPath::Obstructed()
{   
   //ASSERT(_terrain && path_);
   const std::vector<csg::Point3> &points = path_->GetPoints();

   // xxx: IGNORE_OBSTRUCTED:
   return false;
   //return pursuing_ < points.size() && !_grid->isPassable(points[pursuing_]);
}

void FollowPath::Stop()
{
   Report("stopping");
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      mob->SetMoving(false);
      entity_.reset();
   }
}

void FollowPath::Report(std::string msg)
{
   auto entity = entity_.lock();
   if (entity) {
      auto const& points = path_->GetPoints();
      if (!points.empty()) {
         auto start = points.front();
         auto end = points.back();
         auto location = entity->GetComponent<om::Mob>()->GetWorldLocation();
         FP_LOG(5) << msg << " (entity:" << entity->GetObjectId() << " " << start << " -> " << end << " currently at " << location << ")";
      } else {
         FP_LOG(5) << msg << " ( entity:" << entity->GetObjectId() << " path has no points)";
      }
   }
}
