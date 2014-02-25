#include "pch.h"
#include "target_table_entry.ridl.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, TargetTableEntry const& o)
{
   return (os << "[TargetTableEntry]");
}

void TargetTableEntry::ConstructObject()
{
   Record::ConstructObject();
   expire_time_ = 0;
   value_ = 0;
}


bool TargetTableEntry::Update(int now, int interval)
{
   if (*expire_time_ > 0 && now > *expire_time_) {
      value_ = 0;
   }
   return *value_ > 0;
}
