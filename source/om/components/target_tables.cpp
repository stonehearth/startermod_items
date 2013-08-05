#include "pch.h"
#include "target_tables.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;

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

bool TargetTableEntry::Update(int now, int interval)
{
   if (*expires_ > 0 && now > *expires_) {
      value_ = 0;
   }
   return *value_ > 0;
}

TargetTableEntryPtr TargetTable::AddEntry(om::EntityRef e)
{
   TargetTableEntryPtr entry;

   auto entity = e.lock();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      entry = GetEntry(id);
      if (!entry) {
         entry = GetStore().AllocObject<TargetTableEntry>();
         entry->SetTarget(entity);
         entries_[id] = entry;
      }
   }

   return entry;
}

TargetTablePtr TargetTableGroup::AddTable()
{
   TargetTablePtr table = GetStore().AllocObject<TargetTable>();
   table->SetCategory(*category_);
   tables_.Insert(table);
   return table;
}

void TargetTableGroup::RemoveTable(TargetTablePtr table)
{
   tables_.Remove(table);
}


void TargetTable::Update(int now, int interval)
{
   auto i = entries_.begin();
   while (i != entries_.end()) {
      om::TargetTableEntryPtr entry = i->second;
      if (!entry->Update(now, interval)) {
         i = entries_.RemoveIterator(i);
      } else {
         i++;
      }
   }
}

template<class T>
bool pairCompare(const T & x, const T & y) {
   return x.second < y.second; 
}

TargetTableTopPtr TargetTableGroup::GetTop()
{
   std::unordered_map<EntityId, int> values;
   std::unordered_map<EntityId, int> expireTimes;

   for (TargetTablePtr table  : tables_) {
      for (const auto& j: table->GetEntries()) {
         dm::ObjectId id = j.first;
         TargetTableEntryPtr entry = j.second;
         values[id] += entry->GetValue();
         expireTimes[id] = std::max(expireTimes[id], entry->GetExpireTime());
      }
   }
   if (values.empty()) {
      // LOG(WARNING) << "no values at all...";
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
      // LOG(WARNING) << "no target...";
      return nullptr;
   }
   TargetTableTopPtr top = std::make_shared<TargetTableTop>();

   // xxx: RAII
   top->target = target;
   top->value = value;
   top->expires = expireTimes[target->GetObjectId()];

   // LOG(WARNING) << "returning new target table top ptr ..." << top;

   return top;
}

void TargetTableGroup::Update(int now, int interval)
{
   for (TargetTablePtr table : tables_) {
      table->Update(now, interval);
   }
}

TargetTablePtr TargetTables::AddTable(std::string category)
{
   TargetTableGroupPtr group = tables_.Lookup(category, nullptr);
   if (!group) {
      group = GetStore().AllocObject<TargetTableGroup>();
      group->SetCategory(category);
      tables_[category] = group;
   }
   return group->AddTable();
}

void TargetTables::RemoveTable(TargetTablePtr table)
{
   if (table) {
      std::string category = table->GetCategory();
      auto i = tables_.GetContents().find(category);
      if (i != tables_.GetContents().end()) {
         i->second->RemoveTable(table);
      }
   }
}

TargetTableTopPtr TargetTables::GetTop(std::string category)
{
   TargetTableGroupPtr group = tables_.Lookup(category, nullptr);
   if (group) {
      return group->GetTop();
   }
   LOG (WARNING) << "no group of category " << category;
   return nullptr;
}

void TargetTables::Update(int now, int interval)
{
   for (const auto& entry : tables_) {
      entry.second->Update(now, interval);
   }
}
