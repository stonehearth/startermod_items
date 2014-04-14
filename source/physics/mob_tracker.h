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
   TrackerType GetType() const override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;

   csg::Point3 const& GetLastLocation() const;

protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(MobTracker)

private:
   om::MobRef        mob_;
   csg::Point3       last_position_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_MOB_TRACKER_H
