#ifndef _RADIANT_PHYSICS_MOVEMENT_MODIFIER_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_MOVEMENT_MODIFIER_SHAPE_TRACKER_H

#include "region_tracker.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class MovementModifierShapeTracker : public RegionTracker<om::Region3fBoxed>
{
public:
   MovementModifierShapeTracker(NavGrid& ng, om::EntityPtr entity, om::MovementModifierShapePtr dst);
   virtual ~MovementModifierShapeTracker();

   void Initialize() override;   
   float GetPercentBonus();

protected: // RegionTracker implementation
   om::Region3fBoxedPtr GetRegion() const override;

private:
   om::MovementModifierShapeRef      mms_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
