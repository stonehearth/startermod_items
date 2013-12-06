#include "pch.h"
#include "target_table.ridl.h"
#include "target_table_entry.ridl.h"
#include "target_table_group.ridl.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, TargetTableGroup const& o)
{
   return (os << "[TargetTableGroup]");
}

TargetTablePtr TargetTableGroup::AddTable()
{
   TargetTablePtr table = GetStore().AllocObject<TargetTable>();
   table->SetCategory(*category_);
   tables_.Add(table);
   return table;
}

template<class T>
bool pairCompare(const T & x, const T & y) {
   return x.second < y.second; 
}

#if 0
TargetTableTopPtr TargetTableGroup::GetTop()
{
   std::unordered_map<EntityId, int> values(10);
   std::unordered_map<EntityId, int> expireTimes(10);

   for (TargetTablePtr table  : tables_) {
      for (const auto& j: table->GetEntries()) {
         dm::ObjectId id = j.first;
         TargetTableEntryPtr entry = j.second;
         values[id] += entry->GetValue();
         expireTimes[id] = std::max(expireTimes[id], entry->GetExpireTime());
      }
   }
   if (values.empty()) {
      TT_LOG(1) << "no values at all...";
      return nullptr;
   }

   EntityPtr target; 
   int value;
   while (!target && !values.empty()) {
      auto i = std::max_element(values.begin(), values.end(), pairCompare<std::unordered_map<EntityId, int>::value_type>);
      target = GetStore().FetchObject<Entity>(i->first);
      value = i->second;
      values.erase(i);
   }

   if (!target) {
      TT_LOG(1) << "no target...";
      return nullptr;
   }
   TargetTableTopPtr top = std::make_shared<TargetTableTop>();

   // xxx: RAII
   top->target = target;
   top->value = value;
   top->expires = expireTimes[target->GetObjectId()];

   TT_LOG(3) << "returning new target table top ptr ..." << top;

   return top;
}
#endif

void TargetTableGroup::Update(int now, int interval)
{
   for (TargetTablePtr table : tables_) {
      table->Update(now, interval);
   }
}
