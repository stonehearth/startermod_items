#ifndef _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
#define _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H

#include "region_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- RegionCollisionShapeTracker
 *
 * A CollisionTracker for RegionCollisionShape components of Entities.  See collision_tracker.h
 * for more details.
 */
class RegionCollisionShapeTracker : public RegionTracker
{
public:
   RegionCollisionShapeTracker(NavGrid& ng, om::EntityPtr entity, om::RegionCollisionShapePtr rcs);
   virtual ~RegionCollisionShapeTracker() { }

   void Initialize() override;
   TrackerType GetType() const override;

protected:
   om::Region3BoxedPtr GetRegion() const override;

private:
   NO_COPY_CONSTRUCTOR(RegionCollisionShapeTracker)

private:
   om::RegionCollisionShapeRef  rcs_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_REGION_COLLISION_SHAPE_TRACKER_H
