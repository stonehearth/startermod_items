#include "pch.h"
#include "goto_location.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define G_LOG(level)      LOG_CATEGORY(simulation.goto_location, level, GetName())

std::ostream& simulation::operator<<(std::ostream& os, GotoLocation const& o)
{
   return os << "[GotoLocation ...]";
}

GotoLocation::GotoLocation(Simulation& sim, om::EntityRef entity, float speed, const csg::Point3f& location, float stop_distance, luabind::object arrived_cb) :
   Task(sim, "goto location"),
   entity_(entity),
   target_location_(location),
   speed_(speed),
   target_is_entity_(false),
   stop_distance_(stop_distance),
   arrived_cb_(arrived_cb)
{
   if (csg::IsZero(speed)) {
      speed_ = 1.0f;
   }
   Report("constructor");
}

GotoLocation::GotoLocation(Simulation& sim, om::EntityRef entity, float speed, const om::EntityRef target, float stop_distance, luabind::object arrived_cb) :
   Task(sim, "goto entity"),
   entity_(entity),
   target_entity_(target),
   speed_(speed),
   target_is_entity_(true),
   stop_distance_(stop_distance),
   arrived_cb_(arrived_cb)
{
   if (csg::IsZero(speed)) {
      speed_ = 1.0f;
   }
   Report("constructor");
}

static float angle(const csg::Point3f &v)
{
   return (float)(atan2(v.z, -v.x) - atan2(-1, 0));
}

bool GotoLocation::Work(const platform::timer &timer)
{
   Report("running");

   auto entity = entity_.lock();
   if (!entity) {
      return false;
   }

   if (target_is_entity_) {
      auto target = target_entity_.lock();
      if (!target) {
         return false;
      }
      auto mob = target->GetComponent<om::Mob>();
      if (!mob) {
         return false;
      }
      target_location_ = mob->GetLocation();
   }

   // stepSize should be no more than 1.0 to prevent skipping a voxel in the navgrid test
   float const stepSize = 1.0;
   float const maxDistance = speed_ * GetSim().GetBaseWalkSpeed();
   float remainingDistance = maxDistance;
   auto mob = entity->GetComponent<om::Mob>();
   bool finished = false;
   csg::Point3f direction;

   while (!finished && remainingDistance > 0) {
      csg::Point3f const current_location = mob->GetLocation();

      // project the target location to our current standing location
      // we're just walking towards x,z and ignoring y
      target_location_.y = current_location.y;

      direction = csg::Point3f(target_location_ - current_location);
      float const targetDistance = direction.Length() - stop_distance_;
      direction.Normalize();

      float moveDistance = std::min(stepSize, remainingDistance);

      if (targetDistance <= moveDistance) {
         Report("arriving");
         moveDistance = targetDistance;
         finished = true;
      } else {
         Report("moving");
         finished = false;
      }

      csg::Point3f next_location = current_location + direction * moveDistance;
      csg::Point3f resolvedLocation;
      bool passable = MovementHelpers::TestAdjacentMove(GetSim(), entity, true, current_location, next_location, resolvedLocation);

      if (passable) {
         mob->MoveTo(resolvedLocation);
      } else {
         Report("unpassable");
         finished = true;
      }

      remainingDistance -= stepSize;
   }

   mob->TurnTo(angle(direction) * 180 / csg::k_pi);

   if (finished && luabind::type(arrived_cb_) == LUA_TFUNCTION) {
      luabind::call_function<void>(arrived_cb_);
   }
   return !finished;
}

void GotoLocation::Stop()
{
   Report("stopping");
   entity_.reset();
}

void GotoLocation::Report(std::string msg)
{
   auto entity = entity_.lock();
   if (entity) {
      auto location = entity->GetComponent<om::Mob>()->GetWorldLocation();
      G_LOG(5) << msg << " (entity:" << entity->GetObjectId() << " " << target_location_ << " currently at " << location << 
         ".  close to:" << stop_distance_ << "  current d:" << target_location_.DistanceTo(location) <<  ")";
   }
}
