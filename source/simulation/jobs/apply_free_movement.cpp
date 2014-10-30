#include "pch.h"
#include "apply_free_movement.h"
#include "csg/util.h"
#include "csg/transform.h"
#include "om/entity.h"
#include "physics/octtree.h"
#include "physics/nav_grid.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define FM_LOG(level)      LOG_CATEGORY(simulation.free_motion, level, BUILD_STRING(*entity_.lock()))

ApplyFreeMotionTask::ApplyFreeMotionTask(Simulation& sim, om::EntityRef e) :
   Task(sim, "apply free movement"),
   entity_(e)
{
   FM_LOG(5) << "creating free motion task";
}

ApplyFreeMotionTask::~ApplyFreeMotionTask()
{
   FM_LOG(5) << "free motion task destroyed";
}

bool ApplyFreeMotionTask::Work(platform::timer const& timer)
{
   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob->GetInFreeMotion()) {
         // Accleration due to gravity is 9.8 m/(s*s).  One block is one meter.
         // You do the math (oh wait.  there isn't any! =)
         float acceleration = 9.8f / static_cast<float>(GetSim().GetGameTickInterval());

         // Update velocity.  Terminal velocity is currently 1-block per tick
         // to make it really easy to figure out where the thing lands.
         csg::Transform velocity = mob->GetVelocity();
         
         FM_LOG(5) << "adding " << acceleration << " to current velocity " << velocity;

         velocity.position.y -= acceleration;
         velocity.position.y = std::max(velocity.position.y, -1.0);

         // Update position
         bool finished = false;
         phys::NavGrid &ng = GetSim().GetOctTree().GetNavGrid();
         csg::Transform next, current = mob->GetTransform();

         next.position = current.position + velocity.position;
         next.orientation = current.orientation;

         // If our next position is blocked, fall to the bottom of the current
         // brick and clear the free motion flag.
         if (ng.IsBlocked(entity, csg::ToInt(next.position))) {
            FM_LOG(5) << "next position " << next.position << " is blocked.  leaving free motion";

            velocity.position = csg::Point3f::zero;
            next.position.y = std::floor(current.position.y);
            mob->SetInFreeMotion(false);
            finished = true;
         }

         // Update our actual velocity and position.  Return false if we left
         // the free motion state to get the task pruned
         mob->SetVelocity(velocity);
         mob->SetTransform(next);
         return !finished;
      }
   }
   return true;
}
