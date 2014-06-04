#include "radiant.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "region_tracker.h"
#include "physics_util.h"

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
      bounds = LocalToWorld(bounds, GetEntity());
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
      csg::Region3 r = LocalToWorld(region->Get(), GetEntity());
      return r & bounds;
   }
   return csg::Region3::empty;
}


/*
 * -- RegionTracker::GetLocalRegion
 *
 * Return region tracked by this CollisionTracker in the coordinate system of the
 * owning entity.
 *
 */
csg::Region3 const& RegionTracker::GetLocalRegion() const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      return region->Get();
   }
   return csg::Region3::empty;
}

/*
 * -- RegionTracker::Intersects
 *
 * Return whether or not the specified `worldBounds` overlaps with this entity.
 */
bool RegionTracker::Intersects(csg::Cube3 const& worldBounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Region3 r = LocalToWorld(region->Get(), GetEntity());
      r.Intersects(worldBounds);
   }
   return false;
}
