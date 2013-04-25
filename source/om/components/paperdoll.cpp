#include "pch.h"
#include "paperdoll.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Paperdoll::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("slots", slots_);
}
