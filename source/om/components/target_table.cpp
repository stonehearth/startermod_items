#include "pch.h"
#include "target_table.ridl.h"
#include "target_table_entry.ridl.h"
#include "target_table_group.ridl.h"
#include "om/entity.h"
#include <algorithm>

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, TargetTable const& o)
{
   return (os << "[TargetTable]");
}

void TargetTable::LoadFromJson(json::Node const& node)
{
}

void TargetTable::SerializeToJson(json::Node& node) const
{
   Record::SerializeToJson(node);

   for (auto const& entry : entries_) {
      json::Node e;
      entry.second->SerializeToJson(e);
      node.set(stdutil::ToString(entry.first), e);
   }
}

TargetTableEntryPtr TargetTable::AddEntry(om::EntityRef e)
{
   TargetTableEntryPtr entry;

   // The fix for this is for the target table component to register for an event that
   // will get fired every game loop and calls Update(now, interval).  We can add as 
   // a component on the root object, but there's currently no way to fetch the root
   // entity from the store! --tony
   TT_LOG(5) << "FYI!! Target tables are currently somewhat broken (they don't update...)";

   auto entity = e.lock();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      entry = GetEntry(id);
      if (!entry) {
         entry = GetStore().AllocObject<TargetTableEntry>();
         entry->SetTarget(entity);
         entries_.Add(id, entry);
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
         i = entries_.Remove(i);
      } else {
         i++;
      }
   }
}
