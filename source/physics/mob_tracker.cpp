#include "radiant.h"
#include "om/components/mob.ridl.h"
#include "csg/util.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "mob_tracker.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG(physics.navgrid, level)

/*
 * -- MobTracker::MobTracker
 *
 * Track the Mob for an Entity.
 */
MobTracker::MobTracker(NavGrid& ng, om::EntityPtr entity, om::MobPtr mob) :
   CollisionTracker(ng, MOB, entity),
   last_bounds_(csg::CollisionBox::zero),
   mob_(mob)
{
}


/* 
 * -- MobTracker::~MobTracker
 *
 * Destroy the MobTracker.  Ask the NavGrid to mark the tiles which used
 * to contain us as dirty.  They'll notice their weak_ptr's have expired and remove our
 * bits from the vectors.
 *
 */
MobTracker::~MobTracker()
{
   GetNavGrid().OnTrackerDestroyed(last_bounds_, GetEntityId(), GetType());
}

/*
 * -- MobTracker::Initialize
 *
 * Put a trace on the region for the Mob to notify the NavGrid
 * whenever the collision shape changes.
 *
 */
void MobTracker::Initialize()
{
   CollisionTracker::Initialize();
   MarkChanged();
}

/*
 * -- MobTracker::MarkChanged
 *
 * Notify the NavGrid that our shape has changed.  We pass in the current bounds and bounds
 * of the previous shape so the NavGrid can register/un-register us with each tile.
 *
 */
void MobTracker::MarkChanged()
{
   om::EntityPtr entity = GetEntity();
   om::MobPtr mob = mob_.lock();
   if (entity && mob) {
      worldShape_ = csg::ToFloat(mob->GetMobCollisionRegion().Translated(mob->GetWorldGridLocation()));
      bounds_ = worldShape_.GetBounds();
      if (bounds_ != last_bounds_) {
         NG_LOG(9) << "MobTracker for " << *entity << " changed (bounds:" << bounds_ << " last bounds:" << last_bounds_ << ")";
         GetNavGrid().OnTrackerBoundsChanged(last_bounds_, bounds_, shared_from_this());
         last_bounds_ = bounds_;
      } else {
         NG_LOG(9) << "skipping MobTracker bookkeeping for " << *entity << " (bounds:" << bounds_ << " == last bounds:" << last_bounds_ << ")";
      }
   }
}

/*
 * -- MobTracker::GetOverlappingRegion
 *
 * Return the part of our region which overlaps the specified bounds.  Bounds are in
 * world space coordinates!
 *
 */
csg::Region3 MobTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   return csg::ToInt(worldShape_) & bounds;
}

/*
 * -- MobTracker::Intersects
 *
 * Return whether or not the specified `worldBounds` overlaps with this entity.
 */
bool MobTracker::Intersects(csg::CollisionBox const& worldBounds) const
{
   om::MobPtr mob = mob_.lock();
   if (!mob) {
      return false;
   }
   return worldShape_.Intersects(worldBounds);
}
