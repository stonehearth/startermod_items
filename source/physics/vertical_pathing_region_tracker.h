#ifndef _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
#define _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"
#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- VerticalPathingRegionTracker
 *
 * A CollisionTracker for VerticalPathingRegion components of Entities.  See collision_tracker.h
 * for more details.
 */
class VerticalPathingRegionTracker : public RegionTracker
{
public:
   VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr rcs);
   virtual ~VerticalPathingRegionTracker() { }

   void Initialize() override;
   TrackerType GetType() const override;

protected:
   om::Region3BoxedPtr GetRegion() const override;

private:
   NO_COPY_CONSTRUCTOR(VerticalPathingRegionTracker)

private:
   om::VerticalPathingRegionRef  vpr_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_VERTICAL_PATHING_REGION_TRACKER_H
