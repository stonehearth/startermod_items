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

void UnitInfo::ExtendObject(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();

   name_ = obj.get<std::string>("name", *name_);
   description_ = obj.get<std::string>("description", *description_);
   faction_ = obj.get<std::string>("faction", *faction_);
}
