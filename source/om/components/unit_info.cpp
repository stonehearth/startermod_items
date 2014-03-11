#include "pch.h"
#include "unit_info.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, UnitInfo const& o)
{
   return (os << "[UnitInfo " << o.GetDisplayName() << "]");
}

void UnitInfo::LoadFromJson(json::Node const& obj)
{
   display_name_ = obj.get<std::string>("name", *display_name_);
   description_ = obj.get<std::string>("description", *description_);
   character_sheet_info_ = obj.get<std::string>("character_sheet_info", *character_sheet_info_);
   portrait_ = obj.get<std::string>("portrait", *portrait_);
   faction_ = obj.get<std::string>("faction", *faction_);
   kingdom_ = obj.get<std::string>("kingdom", *kingdom_);
   icon_ = obj.get<std::string>("icon", *icon_);
}

void UnitInfo::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("name", GetDisplayName());
   node.set("description", GetDescription());
   node.set("character_sheet_info", GetCharacterSheetInfo());
   node.set("portrait", GetPortrait());
   node.set("faction", GetFaction());
   node.set("kingdom", GetKingdom());
   node.set("icon", GetIcon());
}
