#include "pch.h"
#include "sphere_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void SphereCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("sphere_", sphere_);
}


math3d::aabb SphereCollisionShape::GetAABB() const
{
   ASSERT(false);
   return math3d::aabb();
}

const math3d::bounding_sphere& SphereCollisionShape::GetSphere() const
{
   return *sphere_;
}

