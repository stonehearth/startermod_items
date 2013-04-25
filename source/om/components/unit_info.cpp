#include "pch.h"
#include "unit_info.h"

using namespace ::radiant;
using namespace ::radiant::om;

void UnitInfo::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("name", name_);
   AddRecordField("description", description_);
   AddRecordField("faction", faction_);
}
