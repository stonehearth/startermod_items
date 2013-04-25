#include "pch.h"
#include "follow_path.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/carry_block.h"
#include "simulation/script/script_host.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

extern po::variables_map configvm;

FollowPath::FollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance) :
   Job("follow path"),
   entity_(entity),
   path_(path),
   pursuing_(0),
   finished_(false),
   speed_(speed),
   lateUpdateTime_(0),
   close_to_distance_(close_to_distance)
{
   Report("constructor");
}

static float angle(const math3d::vector3 &v)
{
   return math3d::atan2(v.z, -v.x) - math3d::atan2(-1, 0);
}

bool FollowPath::IsIdle(int now) const
{
   ASSERT(now >= lateUpdateTime_);
   return now == lateUpdateTime_;
}

void FollowPath::Work(int now, const platform::timer &timer)
{
   ASSERT(!finished_);

   Report("running");

   lateUpdateTime_ = now;

   auto entity = entity_.lock();
   if (!entity) {
      finished_ = true;
      return;
   }

   float speedMultiplier = configvm["game.travel_speed_multiplier"].as<float>();
   float maxDistance = speed_ * speedMultiplier;
   auto mob = entity->GetComponent<om::Mob>();
   const std::vector<math3d::ipoint3> &points = path_->GetPoints();

   while (!Arrived(mob) && !Obstructed() && maxDistance > 0)  {
      const math3d::point3 &current = mob->GetLocation();
      const math3d::point3 &goal = math3d::point3(points[pursuing_]);

      math3d::vector3 direction = math3d::vector3(goal - current);
      float distance = direction.length();
      if (distance < maxDistance) {
         mob->MoveTo(math3d::point3(points[pursuing_]));
         maxDistance -= distance;
         pursuing_++;
      } else {
         mob->MoveTo(current + (direction * (maxDistance / distance)));
         maxDistance = 0;
      }
      mob->TurnToAngle(angle(direction) * 180 / k_pi);
   }
   finished_ = Arrived(mob);
   if (finished_) {
      Report("arrived!");
   }
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
   const std::vector<math3d::ipoint3> &points = path_->GetPoints();

   // xxx: IGNORE_OBSTRUCTED:
   return false;
   //return pursuing_ < points.size() && !_grid->isPassable(points[pursuing_]);
}

void FollowPath::Stop()
{
   Report("stopping");
   finished_ = true;
}

void FollowPath::Report(std::string msg)
{
   auto entity = entity_.lock();
   if (entity) {
      auto start = path_->GetPoints().front();
      auto end = path_->GetPoints().back();
      auto location = entity->GetComponent<om::Mob>()->GetWorldLocation();
      LOG(INFO) << msg << " (follow path " << (void*)this << " entity:" << entity->GetObjectId() << " " << start << " -> " << end << " currently at " << location << ")";
   }
}

luabind::scope FollowPath::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<FollowPath, std::shared_ptr<FollowPath>>(name)
         .def("finished", &FollowPath::IsFinished)
         .def("stop",     &FollowPath::Stop)
      ;
}

