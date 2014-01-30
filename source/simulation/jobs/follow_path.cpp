#include "pch.h"
#include "follow_path.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"
#include "csg/util.h" // xxx: should be csg/csg.h
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define FP_LOG(level)      LOG_CATEGORY(simulation.follow_path, level, GetName())

std::ostream& simulation::operator<<(std::ostream& os, FollowPath const& o)
{
   return os << "[FollowPath ...]";
}

FollowPath::FollowPath(Simulation& sim, om::EntityRef e, float speed, std::shared_ptr<Path> path, float close_to_distance, luabind::object unsafe_arrived_cb) :
   Task(sim, "follow path"),
   entity_(e),
   path_(path),
   pursuing_(0),
   speed_(speed),
   close_to_distance_(close_to_distance)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      mob->SetMoving(false);
   }
   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_arrived_cb.interpreter());  
   arrived_cb_ = luabind::object(cb_thread, unsafe_arrived_cb);

   Report("constructor");
}

FollowPath::~FollowPath()
{
   Report("destroying pathfinder");
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
      if (arrived_cb_.is_valid()) {
         try {
            arrived_cb_();
         } catch (std::exception const& e) {
            LUA_LOG(1) << "exception delivering solved cb: " << e.what();
         }
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
         FP_LOG(5) << msg << " (entity:" << entity->GetObjectId() << " path has no points)";
      }
   }
}
