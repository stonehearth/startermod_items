#ifndef _RADIANT_PHYSICS_COLLISION_TRACKER_H
#define _RADIANT_PHYSICS_COLLISION_TRACKER_H

#include "namespace.h"
#include "csg/cube.h"
#include "om/om.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/* 
 * -- CollisionTracker
 *
 * The CollisionTracker is an abstract base class for tracking navigation information
 * about a game object.  Derived classes need to implement GetOverlappingRegion and
 * MarkChanged, and call the NavGrid's Add*Tracker functions to register themselves
 * with the NavGrid.
 */
class CollisionTracker : public std::enable_shared_from_this<CollisionTracker> {
public:
   CollisionTracker(NavGrid& ng, om::EntityPtr entity);
   virtual ~CollisionTracker() { }

   virtual void Initialize();
   csg::Point3 GetEntityPosition() const;
   om::EntityPtr GetEntity() const;

   virtual csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const = 0;
   virtual void MarkChanged() = 0;
   virtual TrackerType GetType() const = 0;

protected:
   NavGrid& GetNavGrid() const;

private:
   NavGrid&       ng_;
   dm::ObjectId   object_id_;
   om::EntityRef  entity_;
   dm::TracePtr   trace_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_COLLISION_TRACKER_H
