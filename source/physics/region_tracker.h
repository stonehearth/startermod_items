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
class RegionTracker : public CollisionTracker
{
public:
   RegionTracker(NavGrid& ng, TrackerType type, om::EntityPtr entity);
   virtual ~RegionTracker();

   csg::Region3 const& GetLocalRegion() const override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::Cube3 const& worldBounds) const override;

   void SetRegionTrace(om::DeepRegionGuardPtr trace);

protected:
   virtual om::Region3BoxedPtr GetRegion() const = 0;
   void MarkChanged();

private:
   NO_COPY_CONSTRUCTOR(RegionTracker)

private:
   om::DeepRegionGuardPtr        trace_;
   csg::Cube3                    last_bounds_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_TRACKER_H
