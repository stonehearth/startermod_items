#include "pch.h"
#include "unit_info.h"

using namespace ::radiant;
using namespace ::radiant::om;

void UnitInfo::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("name", name_);
   AddRecordField("description", description_);
   AddRecordField("faction", faction_);
}

void UnitInfo::Construct(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();

   SetDisplayName(node["name"].as_string());
   SetDescription(node["description"].as_string());
   SetFaction(json::get<std::string>(node, "faction", ""));
}
