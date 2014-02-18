#include "pch.h"
#include "goto_location.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

#define G_LOG(level)      LOG_CATEGORY(simulation.goto_location, level, GetName())

std::ostream& simulation::operator<<(std::ostream& os, GotoLocation const& o)
{
   return os << "[GotoLocation ...]";
}

GotoLocation::GotoLocation(Simulation& sim, om::EntityRef entity, float speed, const csg::Point3f& location, float close_to_distance, luabind::object arrived_cb) :
   Task(sim, "goto location"),
   entity_(entity),
   target_location_(location),
   speed_(speed),
   target_is_entity_(false),
   close_to_distance_(close_to_distance),
   arrived_cb_(arrived_cb)
{
   if (csg::IsZero(speed)) {
      speed_ = 1.0f;
   }
   Report("constructor");
}

GotoLocation::GotoLocation(Simulation& sim, om::EntityRef entity, float speed, const om::EntityRef target, float close_to_distance, luabind::object arrived_cb) :
   Task(sim, "goto entity"),
   entity_(entity),
   target_entity_(target),
   speed_(speed),
   target_is_entity_(true),
   close_to_distance_(close_to_distance),
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

   auto mob = entity->GetComponent<om::Mob>();
   csg::Point3f current = mob->GetLocation();
   csg::Point3f direction = csg::Point3f(target_location_ - current);

   float maxDistance = speed_ * GetSim().GetBaseWalkSpeed();
   float togo = direction.Length() - close_to_distance_;
   direction.Normalize();

   csg::Point3f desiredLocation, actualLocation;
   float distance;
   bool finished;

   if (togo < maxDistance) {
      Report("arriving");
      distance = togo;
      finished = true;
   } else {
      Report("moving");
      distance = maxDistance;
      finished = false;
   }

   desiredLocation = current + direction * distance;
   bool passable = GetStandableLocation(entity, desiredLocation, actualLocation);

   if (passable) {
      mob->TurnTo(angle(direction) * 180 / csg::k_pi);
      mob->MoveTo(actualLocation);
   } else {
      Report("unpassable");
      finished = true;
   }
   if (finished && luabind::type(arrived_cb_) == LUA_TFUNCTION) {
      luabind::call_function<void>(arrived_cb_);
   }
   return !finished;
}

// This code may find paths that are illegal under the pathfinder.
// TODO: Reconcile this code with standard pathfinding rules.
bool GotoLocation::GetStandableLocation(std::shared_ptr<om::Entity> const& entity, csg::Point3f& desiredLocation, csg::Point3f& actualLocation)
{
   phys::OctTree const& octTree = GetSim().GetOctTree();
   csg::Point3 baseLocation = csg::ToClosestInt(desiredLocation);
   csg::Point3 candidate;
   
   candidate = baseLocation;
   if (octTree.CanStandOn(entity, candidate)) {
      actualLocation = csg::ToFloat(candidate);
      return true;
   }

   candidate = baseLocation + csg::Point3::unitY;
   if (octTree.CanStandOn(entity, candidate)) {
      actualLocation = csg::ToFloat(candidate);
      return true;
   }

   candidate = baseLocation - csg::Point3::unitY;
   if (octTree.CanStandOn(entity, candidate)) {
      actualLocation = csg::ToFloat(candidate);
      return true;
   }

   return false;
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
         ".  close to:" << close_to_distance_ << "  current d:" << target_location_.DistanceTo(location) <<  ")";
   }
}
