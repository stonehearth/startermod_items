#include "pch.h"
#include "render_rig.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderRig::InitializeRecordFields() 
{
   Component::InitializeRecordFields();
   AddRecordField("rigs", rigs_);
   AddRecordField("scale", scale_);
   AddRecordField("animation_table", animationTable_);
}
