#include "radiant.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

   
/* 
 * -- RegionTracker::RegionTracker
 *
 * Construct a new RegionTracker.
 */
RegionTracker::RegionTracker(NavGrid& ng, om::EntityPtr entity) :
   CollisionTracker(ng, entity),
   last_bounds_(csg::Cube3::zero)
{
}


/* 
 * -- RegionTracker::~RegionTracker
 *
 * Destroy the RegionTracker.  Ask the NavGrid to mark the tiles which used
 * to contain us as dirty.  They'll notice their weak_ptr's have expired and remove our
 * bits from the vectors.
 */
RegionTracker::~RegionTracker()
{
   GetNavGrid().OnTrackerDestroyed(last_bounds_, GetEntityId());
}


/* 
 * -- RegionTracker::SetRegionTrace
 *
 * Put a trace on the Region to notify the NavGrid whenever it's shape changes.
 */
void RegionTracker::SetRegionTrace(om::DeepRegionGuardPtr trace)
{
   CollisionTracker::Initialize();
   trace_ = trace->OnModified([this]() {
                     MarkChanged();
                  })
                  ->PushObjectState();
}

/*
 * RegionTracker::MarkChanged
 *
 * Notify the NavGrid that our shape has changed.  We pass in the current bounds and bounds
 * of the previous shape so the NavGrid can register/un-register us with each tile.
 */
void RegionTracker::MarkChanged()
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Cube3 bounds = region->Get().GetBounds();
      bounds.Translate(GetEntityPosition());
      GetNavGrid().OnTrackerBoundsChanged(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

/*
 * RegionTracker::GetOverlappingRegion
 *
 * Return the part of our region which overlaps the specified bounds.  Bounds are in
 * world space coordinates, so be sure to transform the region before clipping!
 */
csg::Region3 RegionTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      return region->Get().Translated(GetEntityPosition()) & bounds;
   }
   return csg::Region3::empty;
}

bool RegionTracker::Intersects(csg::Cube3 const& worldBounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Point3 origin = GetEntityPosition();
      return region->Get().Intersects(worldBounds.Translated(-origin));
   }
   return false;
}
