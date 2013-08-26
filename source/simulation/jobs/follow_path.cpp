#include "pch.h"
#include "follow_path.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "simulation/script/script_host.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

extern po::variables_map configvm;

FollowPath::FollowPath(om::EntityRef e, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object arrived_cb) :
   Task("follow path"),
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

   float speedMultiplier = configvm["game.travel_speed_multiplier"].as<float>();
   float maxDistance = speed_ * speedMultiplier;
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
      mob->TurnToAngle(angle(direction) * 180 / csg::k_pi);
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
         .def("stop",     &FollowPath::Stop)
      ;
}

