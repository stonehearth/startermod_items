#include "pch.h"
#include "om/components/target_tables.ridl.h"

using namespace radiant;
using namespace radiant::om;

std::ostream& operator<<(std::ostream& os, TargetTables const& o)
{
   return (os << "[TargetTables]");
}

void TargetTables::ExtendObject(json::Node const& obj)
{
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
