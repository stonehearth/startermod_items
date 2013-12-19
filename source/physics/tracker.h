#ifndef _RADIANT_PHYSICS_TRACKER_H
#define _RADIANT_PHYSICS_TRACKER_H

#include "namespace.h"
#include "csg/cube.h"
#include "om/om.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
class Tracker : public std::enable_shared_from_this<Tracker> {
public:
   Tracker(NavGrid& ng, om::EntityPtr entity);
   void MarkChanged();

protected:
   NavGrid& GetNavGrid() const;
   om::EntityPtr GetEntity() const;

private:
   NavGrid&       ng_;
   om::EntityRef  entity_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_TERRIN_TRACKER_H
