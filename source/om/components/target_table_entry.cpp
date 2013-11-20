#include "pch.h"
#include "target_table_entry.ridl.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;


#if 0
std::ostream& om::operator<<(std::ostream& os, const TargetTableEntry& o)
{
   auto entity = o.GetTarget().lock();
   if (!entity) {
      return (os << "[TargetTableEntry null]");
   }
   return (os << "[TargetTableEntry entity:" << entity->GetObjectId() << " value:" << o.GetValue() << "]");
}

std::ostream& om::operator<<(std::ostream& os, const TargetTable& o)
{
   return (os << "[TargetTable category:" << o.GetCategory() << " name:" << o.GetName());
}

std::ostream& om::operator<<(std::ostream& os, const TargetTableTop& o)
{
   auto entity = o.target.lock();
   if (!entity) {
      return (os << "[TargetTableEntry null]");
   }
   return (os << "[TargetTableEntry entity:" << entity->GetObjectId() << " value:" << o.value << "]");
}

std::ostream& om::operator<<(std::ostream& os, const TargetTableGroup& o)
{
   return (os << "[TargetTableGroup category:" << o.GetCategory() << " #:" << o.GetTableCount() << "]");
}
#endif

void TargetTableEntry::ConstructObject()
{
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
