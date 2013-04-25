#include "pch.h"
#include "action_list.h"

using namespace ::radiant;
using namespace ::radiant::om;

void ActionList::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("actions", actions_);
}
