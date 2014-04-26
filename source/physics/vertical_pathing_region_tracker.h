#ifndef _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
#define _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- VerticalPathingRegionTracker
 *
 * A CollisionTracker for VerticalPathingRegion components of Entities.  See collision_tracker.h
 * for more details.
 */
class VerticalPathingRegionTracker : public CollisionTracker
{
public:
   VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr rcs);
   ~VerticalPathingRegionTracker();

   void Initialize() override;
   TrackerType GetType() const override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::Cube3 const& worldBounds) const override;

private:
   om::Region3BoxedPtr GetRegion() const;
   void MarkChanged();

private:
   NO_COPY_CONSTRUCTOR(VerticalPathingRegionTracker)

private:
   om::VerticalPathingRegionRef  vpr_;
   om::DeepRegionGuardPtr        trace_;
   csg::Cube3                    last_bounds_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
