#include "pch.h"
#include "region_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RegionCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

csg::Cube3f RegionCollisionShape::GetAABB() const
{
   ASSERT(false);
   return csg::Cube3f();
}
