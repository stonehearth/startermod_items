#include "radiant.h"
#include "csg/util.h"
#include "csg/point.h"
#include "om/region.h"
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
template <typename BoxedRegion>
RegionTracker<BoxedRegion>::RegionTracker(NavGrid& ng, TrackerType type, om::EntityPtr entity) :
   CollisionTracker(ng, type, entity),
   last_bounds_(csg::CollisionBox::zero)
{
}


/* 
 * -- RegionTracker::~RegionTracker
 *
 * Destroy the RegionTracker.  Ask the NavGrid to mark the tiles which used
 * to contain us as dirty.  They'll notice their weak_ptr's have expired and remove our
 * bits from the vectors.
 */
template <typename BoxedRegion>
RegionTracker<BoxedRegion>::~RegionTracker()
{
   GetNavGrid().OnTrackerDestroyed(last_bounds_, GetEntityId(), GetType());
}

/* 
 * -- RegionTracker::SetRegionTrace
 *
 * Put a trace on the Region to notify the NavGrid whenever it's shape changes.
 */
template <typename BoxedRegion>
void RegionTracker<BoxedRegion>::SetRegionTrace(RegionTracePtr trace)
{
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
template <typename BoxedRegion>
void RegionTracker<BoxedRegion>::MarkChanged()
{
   BoxedRegionPtr region = GetRegion();
   if (region) {
      Cube bounds = region->Get().GetBounds();
      bounds = LocalToWorld(bounds, GetEntity());
      csg::CollisionBox newBounds = csg::ToFloat(bounds);
      GetNavGrid().OnTrackerBoundsChanged(last_bounds_, newBounds, shared_from_this());
      last_bounds_ = newBounds;
   }
}

/*
 * RegionTracker::GetOverlappingRegion
 *
 * Return the part of our region which overlaps the specified bounds.  Bounds are in
 * world space coordinates, so be sure to transform the region before clipping!
 */
template <typename BoxedRegion>
csg::Region3 RegionTracker<BoxedRegion>::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   BoxedRegionPtr region = GetRegion();
   if (region) {
      csg::Region3 r = csg::ToInt(region->Get());

      return LocalToWorld(r, GetEntity()) & bounds;
   }
   return csg::Region3::zero;
}

/*
 * -- RegionTracker::Intersects
 *
 * Return whether or not the specified `worldBounds` overlaps with this entity.
 */
template <typename BoxedRegion>
bool RegionTracker<BoxedRegion>::Intersects(csg::CollisionBox const& worldBounds) const
{
   BoxedRegionPtr region = GetRegion();
   if (region) {
      Region r = LocalToWorld(region->Get(), GetEntity());
      return r.Intersects(csg::ConvertTo<ScalarType, 3>(worldBounds));
   }
   return false;
}

#define MAKE_REGION_TRACKER(Cls) \
   template Cls::RegionTracker(NavGrid& ng, TrackerType type,  om::EntityPtr entityPtr); \
   template Cls::~RegionTracker(); \
   template void Cls::SetRegionTrace(RegionTracePtr trace); \
   template void Cls::MarkChanged(); \
   template csg::Region3 Cls::GetOverlappingRegion(csg::Cube3 const& bounds) const; \
   template bool Cls::Intersects(csg::CollisionBox const& worldBounds) const; \

MAKE_REGION_TRACKER(RegionTracker<om::Region3Boxed>)
MAKE_REGION_TRACKER(RegionTracker<om::Region3fBoxed>)
