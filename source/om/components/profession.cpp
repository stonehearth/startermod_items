#include "pch.h"
#include "profession.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Profession::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("learned_recpies", learnedReceipes_);
}
