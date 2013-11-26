#include "pch.h"
#include "unit_info.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, UnitInfo const& o)
{
   return (os << "[UnitInfo " << o.GetDisplayName() << "]");
}

void UnitInfo::ExtendObject(json::Node const& obj)
{
   JSONNode const& node = obj.GetNode();

   display_name_ = obj.get<std::string>("name", *display_name_);
   description_ = obj.get<std::string>("description", *description_);
   faction_ = obj.get<std::string>("faction", *faction_);
   icon_ = obj.get<std::string>("icon", *icon_);
}
