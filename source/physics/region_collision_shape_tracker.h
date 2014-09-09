#ifndef _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H

#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class RegionCollisionShapeTracker : public RegionTracker<om::Region3fBoxed>
{
public:
   RegionCollisionShapeTracker(NavGrid& ng, TrackerType t, om::EntityPtr entity, om::RegionCollisionShapePtr dst);
   virtual ~RegionCollisionShapeTracker();

   void Initialize() override;   

protected: // RegionTracker implementation
   om::Region3fBoxedPtr GetRegion() const override;

private:
   om::RegionCollisionShapeRef      rcs_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
