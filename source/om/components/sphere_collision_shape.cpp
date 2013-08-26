#include "pch.h"
#include "sphere_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void SphereCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("sphere_", sphere_);
}


csg::Cube3f SphereCollisionShape::GetAABB() const
{
   ASSERT(false);
   return csg::Cube3f();
}

const csg::Sphere& SphereCollisionShape::GetSphere() const
{
   return *sphere_;
}

