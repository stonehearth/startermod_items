#ifndef _RADIANT_PHYSICS_DESTINATION_REGION_TRACKER_H
#define _RADIANT_PHYSICS_DESTINATION_REGION_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"
#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- DestinationRegionTracker
 *
 * A CollisionTracker for DestinationRegion components of Entities.  See collision_tracker.h
 * for more details.
 */
class DestinationRegionTracker : public RegionTracker
{
public:
   DestinationRegionTracker(NavGrid& ng, om::EntityPtr entity, om::DestinationPtr dst);
   virtual ~DestinationRegionTracker() { }

   void Initialize() override;
   TrackerType GetType() const override;

protected:
   om::Region3BoxedPtr GetRegion() const override;

private:
   NO_COPY_CONSTRUCTOR(DestinationRegionTracker)

private:
   om::DestinationRef  dst_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_DESTINATION_REGION_TRACKER_H
