#ifndef _RADIANT_PHYSICS_FREE_MOTION_H
#define _RADIANT_PHYSICS_FREE_MOTION_H

#include "namespace.h"
#include "csg/point.h"
#include "om/om.h"
#include "core/guard.h"
#include "platform/utils.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class FreeMotion {
public:
   FreeMotion(NavGrid &ng);

   void ProcessDirtyTiles(platform::timer& t);

private:
   void ProcessDirtyTile(csg::Point3 const& index);
   void UnstickEntity(om::EntityPtr entity);

private:
   NavGrid&                   _ng;
   std::vector<csg::Point3>   _dirtyTiles;
   core::Guard                _guard;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_FREE_MOTION_H
 