#ifndef _RADIANT_PHYSICS_MOB_TRACKER_H
#define _RADIANT_PHYSICS_MOB_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- MobTracker
 *
 * A CollisionTracker for Mob components of Entities.  See collision_tracker.h
 * for more details.
 */
class MobTracker : public CollisionTracker
{
public:
   MobTracker(NavGrid& ng, om::EntityPtr entity, om::MobPtr mob);
   ~MobTracker();

   void Initialize() override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::CollisionBox const& worldBounds) const override;
   void ClipRegion(csg::CollisionShape& region) const override { };

   csg::CollisionShape const& GetWorldShape() const { return worldShape_; }
protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(MobTracker)

private:
   om::MobRef           mob_;
   csg::CollisionBox    bounds_;
   csg::CollisionBox    last_bounds_;
   csg::CollisionShape  worldShape_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_MOB_TRACKER_H
