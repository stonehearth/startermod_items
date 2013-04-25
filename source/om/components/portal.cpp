#include "pch.h"
#include "portal.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Portal::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void Portal::SetPortal(const resources::Region2d& region)
{
   region_ = region.GetRegion();
}
