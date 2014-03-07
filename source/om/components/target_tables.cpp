#include "pch.h"
#include "om/components/target_tables.ridl.h"

using namespace radiant;
using namespace radiant::om;

std::ostream& operator<<(std::ostream& os, TargetTables const& o)
{
   return (os << "[TargetTables]");
}

void TargetTables::LoadFromJson(json::Node const& obj)
{
}

void TargetTables::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   for (auto const& entry : tables_) {
      json::Node table;
      entry.second->SerializeToJson(table);
      node.set(stdutil::ToString(entry.first), table);
   }
}

TargetTablePtr TargetTables::AddTable(std::string const& category)
{
   TargetTableGroupPtr group = tables_.Get(category, nullptr);
   if (!group) {
      group = GetStore().AllocObject<TargetTableGroup>();
      group->SetCategory(category);
      tables_.Add(category, group);
   }
   return group->AddTable();
}
