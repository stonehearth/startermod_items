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

BoxedRegion3Ptr RenderRegion::GetRegion() const
{
   return *region_;
}

void RenderRegion::SetRegion(BoxedRegion3Ptr r)
{
   region_ = r;
}

dm::Guard RenderRegion::TraceRenderRegion(const char* reason,
                                          std::function<void(csg::Region3 const&)> cb) const
{
   return TraceBoxedRegion3PtrField(region_, reason, cb);
}
