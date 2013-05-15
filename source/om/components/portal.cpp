#include "pch.h"
#include "portal.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Portal::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void Portal::SetPortal(csg::Region2 const& region)
{
   region_ = region;
}
