#include "radiant.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "vertical_pathing_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

   
/* 
 * -- VerticalPathingRegionTracker::VerticalPathingRegionTracker
 *
 * Construct a new VerticalPathingRegionTracker.
 */
VerticalPathingRegionTracker::VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr vpr) :
   CollisionTracker(ng, entity),
   vpr_(vpr),
   last_bounds_(csg::Cube3::zero)
{
}


/* 
 * -- VerticalPathingRegionTracker::~VerticalPathingRegionTracker
 *
 * Destroy the VerticalPathingRegionTracker.  Ask the NavGrid to mark the tiles which used
 * to contain us as dirty.  They'll notice their weak_ptr's have expired and remove our
 * bits from the vectors.
 */
VerticalPathingRegionTracker::~VerticalPathingRegionTracker()
{
   GetNavGrid().MarkDirty(last_bounds_);
}


/* 
 * -- VerticalPathingRegionTracker::Initialize
 *
 * Put a trace on the VerticalPathingRegion to notify the NavGrid whenever it's shape changes.
 */
void VerticalPathingRegionTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      trace_ = vpr->TraceRegion("nav grid", GetNavGrid().GetTraceCategory())
         ->OnModified([this]() {
            MarkChanged();
         })
         ->PushObjectState();
   }
}

/*
 * VerticalPathingRegionTracker::MarkChanged
 *
 * Notify the NavGrid that our shape has changed.  We pass in the current bounds and bounds
 * of the previous shape so the NavGrid can register/un-register us with each tile.
 */
void VerticalPathingRegionTracker::MarkChanged()
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Cube3 bounds = region->Get().GetBounds();
      bounds.Translate(GetEntityPosition());
      GetNavGrid().AddCollisionTracker(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

/*
 * VerticalPathingRegionTracker::GetOverlappingRegion
 *
 * Return the part of our region which overlaps the specified bounds.  Bounds are in
 * world space coordinates, so be sure to transform the region before clipping!
 */
csg::Region3 VerticalPathingRegionTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      return region->Get().Translated(GetEntityPosition()) & bounds;
   }
   return csg::Region3::empty;
}

/*
 * VerticalPathingRegionTracker::GetRegion
 *
 * Helper method to get the current collision shape region.
 */
om::Region3BoxedPtr VerticalPathingRegionTracker::GetRegion() const
{
   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      return vpr->GetRegion();
   }
   return nullptr;
}

TrackerType VerticalPathingRegionTracker::GetType() const
{
   return LADDER;
}
