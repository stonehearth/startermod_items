#ifndef _RADIANT_OM_SPHERE_COLLSION_SHAPE_H
#define _RADIANT_OM_SPHERE_COLLSION_SHAPE_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "namespace.h"
#include "csg/sphere.h"
#include "collision_shape.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class SphereCollisionShape : public Component,
                             public CollisionShape
{
public:
   DEFINE_OM_OBJECT_TYPE(SphereCollisionShape, sphere_collision_shape);

   CollisionType GetType() const override { return CollisionShape::SPHERE; }

   csg::Cube3f GetAABB() const override;
   const csg::Sphere& GetSphere() const;

private:
   void InitializeRecordFields() override;

private:
   dm::Boxed<csg::Sphere>     sphere_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_SPHERE_COLLSION_SHAPE_H
