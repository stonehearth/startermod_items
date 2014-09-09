#ifndef _RADIANT_PHYSICS_REGION_TRACKER_H
#define _RADIANT_PHYSICS_REGION_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "om/region.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- RegionTracker
 *
 * A CollisionTracker for things which need to track Regions.  This template class should be
 * overridden by other classes which know how to deal with each region type.
 */
template <typename BoxedRegion>
class RegionTracker : public CollisionTracker
{
public:
   DECLARE_SHARED_POINTER_TYPES(BoxedRegion);

   typedef typename BoxedRegion::Value       Region;
   typedef typename Region::Cube             Cube;
   typedef typename Region::ScalarType       ScalarType;
   typedef dm::Boxed<BoxedRegionPtr>         OuterBox;
   typedef om::DeepRegionGuard<OuterBox>     RegionTrace;
   DECLARE_SHARED_POINTER_TYPES(RegionTrace);

public:
   RegionTracker(NavGrid& ng, TrackerType type, om::EntityPtr entity);
   virtual ~RegionTracker();

   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::CollisionBox const& worldBounds) const override;

   void SetRegionTrace(RegionTracePtr trace);

protected:
   virtual BoxedRegionPtr GetRegion() const = 0;
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(RegionTracker)

private:
   RegionTracePtr          trace_;
   csg::CollisionBox       last_bounds_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_TRACKER_H
