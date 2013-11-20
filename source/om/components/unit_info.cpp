#include "pch.h"
#include "unit_info.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

void UnitInfo::ExtendObject(json::Node const& obj)
{
   JSONNode const& node = obj.GetNode();

   name_ = obj.get<std::string>("name", *name_);
   description_ = obj.get<std::string>("description", *description_);
   faction_ = obj.get<std::string>("faction", *faction_);
   icon_ = obj.get<std::string>("icon", *icon_);
}
