#ifndef _RADIANT_OM_COLLSION_SHAPE_H
#define _RADIANT_OM_COLLSION_SHAPE_H

#include "dm/record.h"
#include "om/object_enums.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class CollisionShape
{
public:
   virtual ~CollisionShape() { }

   enum CollisionType {
      GRID = 0,
      REGION = 1,
      SPHERE = 2
   };

   virtual CollisionType GetType() const = 0;
   virtual csg::Cube3f GetAABB() const = 0;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COLLSIONS_SHAPE_H
