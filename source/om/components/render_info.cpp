#include "pch.h"
#include "render_info.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderInfo::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("iconic", iconic_);
   iconic_ = false;
}
