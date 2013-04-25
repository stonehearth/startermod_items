#include "pch.h"
#include "vertical_pathing_region.h"

using namespace ::radiant;
using namespace ::radiant::om;

void VerticalPathingRegion::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

math3d::aabb VerticalPathingRegion::GetAABB() const
{
   ASSERT(false);
   return math3d::aabb();
}
