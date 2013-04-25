#include "pch.h"
#include "region_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RegionCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

math3d::aabb RegionCollisionShape::GetAABB() const
{
   ASSERT(false);
   return math3d::aabb();
}
