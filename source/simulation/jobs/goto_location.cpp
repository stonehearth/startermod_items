#include "pch.h"
#include "goto_location.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/carry_block.h"
#include "simulation/script/script_host.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

extern po::variables_map configvm;

GotoLocation::GotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance) :
   Job("goto location"),
   entity_(entity),
   target_location_(location),
   finished_(false),
   speed_(speed),
   lateUpdateTime_(0),
   target_is_entity_(false),
   close_to_distance_(close_to_distance)
{
   if (math3d::is_zero(speed)) {
      speed_ = 1.0f;
   }
   Report("constructor");
}

GotoLocation::GotoLocation(om::EntityRef entity, float speed, const om::EntityRef target, float close_to_distance) :
   Job("goto entity"),
   entity_(entity),
   target_entity_(target),
   finished_(false),
   speed_(speed),
   lateUpdateTime_(0),
   target_is_entity_(true),
   close_to_distance_(close_to_distance)
{
   if (math3d::is_zero(speed)) {
      speed_ = 1.0f;
   }
   Report("constructor");
}

static float angle(const math3d::vector3 &v)
{
   return math3d::atan2(v.z, -v.x) - math3d::atan2(-1, 0);
}

bool GotoLocation::IsIdle(int now) const
{
   ASSERT(now >= lateUpdateTime_);
   return now == lateUpdateTime_;
}

void GotoLocation::Work(int now, const platform::timer &timer)
{
   lateUpdateTime_ = now;

   ASSERT(!finished_);

   Report("running");

   auto entity = entity_.lock();
   if (!entity) {
      finished_ = true;
      return;
   }
   if (target_is_entity_) {
      auto target = target_entity_.lock();
      if (!target) {
         finished_ = true;
         return;
      }
      auto mob = target->GetComponent<om::Mob>();
      if (!mob) {
         finished_ = true;
         return;
      }
      target_location_ = mob->GetLocation();
   }

   auto mob = entity->GetComponent<om::Mob>();
   auto location = mob->GetLocation();

   float speedMultiplier = configvm["game.travel_speed_multiplier"].as<float>();
   float maxDistance = speed_ * speedMultiplier;

   math3d::point3 current = mob->GetLocation();
   math3d::vector3 direction = math3d::vector3(target_location_ - current);

   float togo = direction.length() - close_to_distance_;
   direction.normalize();

   if (togo < maxDistance) {
      finished_  = true;
      mob->MoveTo(current + direction * togo);
   } else {
      mob->TurnToAngle(angle(direction) * 180 / k_pi);
      mob->MoveTo(current + direction * maxDistance);
   }
   Report("moving");
}

void GotoLocation::Stop()
{
   Report("stopping");
   finished_ = true;
}

void GotoLocation::Report(std::string msg)
{
   auto entity = entity_.lock();
   if (entity) {
      auto location = entity->GetComponent<om::Mob>()->GetWorldLocation();
      LOG(INFO) << msg << " (entity " << entity->GetObjectId() << " goto location " << (void*)this << " entity:" << entity->GetObjectId() << " " << target_location_ << " currently at " << location << 
         ".  close to:" << close_to_distance_ << "  current d:" << target_location_.distance(location) <<  ")";
   }
}

luabind::scope GotoLocation::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<GotoLocation, std::shared_ptr<GotoLocation>>(name)
         .def("finished", &GotoLocation::IsFinished)
         .def("stop",     &GotoLocation::Stop)
      ;
}
