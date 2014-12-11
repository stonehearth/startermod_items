#ifndef _RADIANT_PHYSICS_MOVEMENT_GUARD_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_MOVEMENT_GUARD_SHAPE_TRACKER_H

#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class MovementGuardShapeTracker : public RegionTracker<om::Region3fBoxed>
{
public:
   MovementGuardShapeTracker(NavGrid& ng, om::EntityPtr entity, om::MovementGuardShapePtr dst);
   virtual ~MovementGuardShapeTracker();

   void Initialize() override;   
   bool CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& location);

protected: // RegionTracker implementation
   om::Region3fBoxedPtr GetRegion() const override;

private:
   om::MovementGuardShapeRef      mgs_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
