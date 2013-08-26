#include "pch.h"
#include "vertical_pathing_region.h"

using namespace ::radiant;
using namespace ::radiant::om;

void VerticalPathingRegion::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

csg::Cube3f VerticalPathingRegion::GetAABB() const
{
   ASSERT(false);
   return csg::Cube3f();
}
