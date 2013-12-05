#include "pch.h"
#include "follow_path_action.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/carry_block.ridl.ridl.h"
#include <boost/program_options.hpp>

using namespace ::radiant;
using namespace ::radiant::simulation;
namespace po = boost::program_options;

FollowPathAction::FollowPathAction() :
   pursuing_(0)
{
}

static float angle(const csg::Point3f &v)
{
   return atan2(v.z, -v.x) - atan2(-1, 0);
}

void FollowPathAction::Create(lua_State* L)
{
   using namespace luabind;

   int nargs = lua_gettop(L);
   object arg0(from_stack(L, -2));
   object arg1(from_stack(L, -1));

   self_ = object_cast<om::EntityRef>(arg0).lock();
   path_ = object_cast<std::shared_ptr<Path> >(arg1);
   pursuing_ = 0;
}

void FollowPathAction::Start()
{
   auto& script = ScriptHost::GetInstance();
   auto* L = script.GetInterpreter();
   om::EntityPtr self = self_.lock();

   Action::Start();
   auto carryBlock = self->GetComponent<om::CarryBlock>();
   if (carryBlock && carryBlock->GetCarrying().lock()) {
      movingEffect_ = "carry-run";
   } else {
      movingEffect_ = "run";
   }
   script.SendMsg(self, "radiant.animation.start_action", luabind::object(L, movingEffect_));
   mob_ = self->GetComponent<om::Mob>();
}

void FollowPathAction::Stop()
{
   auto& script = ScriptHost::GetInstance();
   auto* L = script.GetInterpreter();
   Action::Stop();
   script.SendMsg(self_.lock(), "radiant.animation.stop_action", luabind::object(L, movingEffect_));
}

void FollowPathAction::Update()
{
   ASSERT(path_);
   
   if (!FollowPath()) {
      return;
   }

   auto location = mob_->GetGridLocation();
   ASSERT(location == path_->GetPoints().back());

   Finish();
}

bool FollowPathAction::FollowPath()
{
   // let's teleport!
   //GetSelf()->MoveToGridAligned(path_->GetDestination());
   //return true;

   float maxDistance = 0.4f;
   const vector<csg::Point3> &points = path_->GetPoints();
   auto self = self_.lock();

   while (!Arrived() && !Obstructed() && maxDistance > 0)  {
      const csg::Point3f &current = mob_->GetLocation();
      const csg::Point3f &goal = csg::Point3f(points[pursuing_]);

      csg::Point3f direction = csg::Point3f(goal - current);
      float distance = direction.length();
      if (distance < maxDistance) {
         mob_->MoveTo(csg::Point3f(points[pursuing_]));
         maxDistance -= distance;
         pursuing_++;
      } else {
         mob_->MoveTo(current + (direction * (maxDistance / distance)));
         maxDistance = 0;
      }
      mob_->TurnToAngle(angle(direction) * 180 / k_pi);
   }

   bool arrived = Arrived();
   if (arrived) {
      return true;
   }
   return Arrived();
}

bool FollowPathAction::Arrived()
{
   ASSERT(path_);
   const vector<csg::Point3> &points = path_->GetPoints();

   return (unsigned int)pursuing_ >= points.size();
}

bool FollowPathAction::Obstructed()
{   
   //ASSERT(_terrain && path_);
   const vector<csg::Point3> &points = path_->GetPoints();

   // xxx: IGNORE_OBSTRUCTED:
   return false;
   //return pursuing_ < points.size() && !_grid->isPassable(points[pursuing_]);
}