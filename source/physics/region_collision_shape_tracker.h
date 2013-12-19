#ifndef _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
class RegionCollisionShapeTracker : public CollisionTracker
{
public:
   RegionCollisionShapeTracker(NavGrid& ng, om::EntityPtr entity, om::RegionCollisionShapePtr rcs);

   void Initialize() override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   void MarkChanged() override;

private:
   om::Region3BoxedPtr GetRegion() const;

private:
   NO_COPY_CONSTRUCTOR(RegionCollisionShapeTracker)

private:
   om::RegionCollisionShapeRef  rcs_;
   om::DeepRegionGuardPtr       trace_;
   csg::Cube3                   last_bounds_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
