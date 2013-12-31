#include "radiant.h"
#include "om/components/region_collision_shape.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "region_collision_shape_tracker.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * RegionCollisionShapeTracker::RegionCollisionShapeTracker
 *
 * Track the RegionCollisionShape for an Entity.
 */
RegionCollisionShapeTracker::RegionCollisionShapeTracker(NavGrid& ng, om::EntityPtr entity, om::RegionCollisionShapePtr rcs) :
   CollisionTracker(ng, entity),
   rcs_(rcs),
   last_bounds_(csg::Cube3::zero)
{
}

/*
 * RegionCollisionShapeTracker::Initialize
 *
 * Put a trace on the region for the RegionCollisionShape to notify the NavGrid
 * whenever the collision shape changes.
 */
void RegionCollisionShapeTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      trace_ = rcs->TraceRegion("nav grid", GetNavGrid().GetTraceCategory())
         ->OnModified([this]() {
            MarkChanged();
         })
         ->PushObjectState();
   }
}

/*
 * RegionCollisionShapeTracker::MarkChanged
 *
 * Notify the NavGrid that our shape has changed.  We pass in the current bounds and bounds
 * of the previous shape so the NavGrid can register/un-register us with each tile.
 */
void RegionCollisionShapeTracker::MarkChanged()
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
 * RegionCollisionShapeTracker::GetOverlappingRegion
 *
 * Return the part of our region which overlaps the specified bounds.  Bounds are in
 * world space coordinates, so be sure to transform the region before clipping!
 */
csg::Region3 RegionCollisionShapeTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      // Move the region to the world-space position of our entity, then clip it
      // to the bounds rectangle.
      return region->Get().Translated(GetEntityPosition()) & bounds;
   }
   return csg::Region3::empty;
}


/*
 * RegionCollisionShapeTracker::GetRegion
 *
 * Helper method to get the current collision shape region.
 */
om::Region3BoxedPtr RegionCollisionShapeTracker::GetRegion() const
{
   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      return rcs->GetRegion();
   }
   return nullptr;
}

TrackerType RegionCollisionShapeTracker::GetType() const
{
   return COLLISION;
}
