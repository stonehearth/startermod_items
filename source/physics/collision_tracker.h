#ifndef _RADIANT_PHYSICS_COLLISION_TRACKER_H
#define _RADIANT_PHYSICS_COLLISION_TRACKER_H

#include "namespace.h"
#include "csg/cube.h"
#include "om/om.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
class CollisionTracker : public std::enable_shared_from_this<CollisionTracker> {
public:
   CollisionTracker(NavGrid& ng, om::EntityPtr entity);

   virtual void Initialize();
   virtual csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const = 0;
   virtual void MarkChanged() = 0;
   csg::Point3 GetEntityPosition() const;

protected:
   NavGrid& GetNavGrid() const;
   om::EntityPtr GetEntity() const;

private:
   NavGrid&       ng_;
   dm::ObjectId   object_id_;
   om::EntityRef  entity_;
   dm::TracePtr   trace_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_COLLISION_TRACKER_H
