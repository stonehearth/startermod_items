#include "radiant.h"
#include "om/components/mob.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "mob_tracker.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG(physics.navgrid, level)

/*
 * MobTracker::MobTracker
 *
 * Track the Mob for an Entity.
 */
MobTracker::MobTracker(NavGrid& ng, om::EntityPtr entity, om::MobPtr mob) :
   CollisionTracker(ng, entity),
   last_bounds_(csg::Point3::zero, csg::Point3::zero),
   mob_(mob)
{
}


/* 
 * -- MobTracker::~MobTracker
 *
 * Destroy the MobTracker.  Ask the NavGrid to mark the tiles which used
 * to contain us as dirty.  They'll notice their weak_ptr's have expired and remove our
 * bits from the vectors.
 */

MobTracker::~MobTracker()
{
   GetNavGrid().MarkDirty(last_bounds_);
}

/*
 * MobTracker::Initialize
 *
 * Put a trace on the region for the Mob to notify the NavGrid
 * whenever the collision shape changes.
 */
void MobTracker::Initialize()
{
   CollisionTracker::Initialize();
   MarkChanged();
}

/*
 * MobTracker::MarkChanged
 *
 * Notify the NavGrid that our shape has changed.  We pass in the current bounds and bounds
 * of the previous shape so the NavGrid can register/un-register us with each tile.
 */
void MobTracker::MarkChanged()
{
   om::MobPtr mob = mob_.lock();
   if (mob) {
      csg::Point3 pos = mob->GetWorldGridLocation();
      csg::Cube3 bounds(pos, pos + csg::Point3(1, 1, 1));
      NG_LOG(9) << "adding MobTracker for " << *mob->GetEntityPtr() << " to tile " << bounds << "(last bounds:" << last_bounds_ << ")";
      GetNavGrid().AddCollisionTracker(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}


/*
 * MobTracker::GetOverlappingRegion
 *
 * Theoretically returns the region which overlaps the specified bounds.  In practice,
 * this is used to update bit-vectors in the NavGridTileData structure, which we don't
 * contribute to.  If this gets called, something terribly, terribly wrong has happened.
 */
csg::Region3 MobTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   NOT_YET_IMPLEMENTED();
   return csg::Region3::empty;
}

TrackerType MobTracker::GetType() const
{
   return MOB;
}

csg::Point3 const& MobTracker::GetLastLocation() const
{
   return last_position_;
}
