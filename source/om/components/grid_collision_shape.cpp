#include "pch.h"
#include "grid_collision_shape.h"
#include "om/grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

void GridCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("grid", grid_);
}

math3d::aabb GridCollisionShape::GetAABB() const
{
   ASSERT(false);
   return math3d::aabb();
}

