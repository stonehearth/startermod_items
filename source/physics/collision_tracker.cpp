#include "radiant.h"
#include "nav_grid.h"
#include "collision_tracker.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

CollisionTracker::CollisionTracker(NavGrid& ng, om::EntityPtr entity) :
   ng_(ng),
   entity_(entity)
{
}

void CollisionTracker::Initialize()
{
   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         trace_  = mob->TraceTransform("nav grid", GetNavGrid().GetTraceCategory())
            ->OnModified([this]() {
               MarkChanged();
            });
      }
   }
}

NavGrid& CollisionTracker::GetNavGrid() const
{
   return ng_;
}

om::EntityPtr CollisionTracker::GetEntity() const
{
   return entity_.lock();
}

csg::Point3 CollisionTracker::GetEntityPosition() const
{
   csg::Point3 position(csg::Point3::zero);
   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         position = mob->GetWorldGridLocation();
      }
   }
   return position;
}
