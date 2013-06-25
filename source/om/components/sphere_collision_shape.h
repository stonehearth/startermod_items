#ifndef _RADIANT_OM_SPHERE_COLLSION_SHAPE_H
#define _RADIANT_OM_SPHERE_COLLSION_SHAPE_H

#include "math3d.h"
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "namespace.h"
#include "math3d/collision/bounding_sphere.h"
#include "collision_shape.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class SphereCollisionShape : public Component,
                             public CollisionShape
{
public:
   DEFINE_OM_OBJECT_TYPE(SphereCollisionShape, sphere_collision_shape);

   CollisionType GetType() const override { return CollisionShape::SPHERE; }

   math3d::aabb GetAABB() const override;
   const math3d::bounding_sphere& GetSphere() const;

private:
   void InitializeRecordFields() override;

private:
   dm::Boxed<math3d::bounding_sphere>     sphere_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_SPHERE_COLLSION_SHAPE_H
