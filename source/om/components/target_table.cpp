#include "pch.h"
#include "target_table.ridl.h"
#include "target_table_entry.ridl.h"
#include "target_table_group.ridl.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;

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
         entries_.Insert(id, entry);
      }
   }

   return entry;
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
