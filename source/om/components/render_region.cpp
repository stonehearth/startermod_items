#include "pch.h"
#include "render_region.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderRegion::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void RenderRegion::SetRegion(Region3BoxedPtr r)
{
   region_ = r;
}
