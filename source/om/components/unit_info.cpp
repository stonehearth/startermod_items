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
   AddRecordField("icon", icon_);
}

void UnitInfo::ExtendObject(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();

   name_ = obj.get<std::string>("name", *name_);
   description_ = obj.get<std::string>("description", *description_);
   faction_ = obj.get<std::string>("faction", *faction_);
   icon_ = obj.get<std::string>("icon", *icon_);
}

dm::Guard UnitInfo::TraceObjectChanges(const char* reason, std::function<void()> fn) const
{
   dm::Guard guard;
   guard += name_.TraceObjectChanges(reason, fn);
   guard += description_.TraceObjectChanges(reason, fn);
   guard += faction_.TraceObjectChanges(reason, fn);
   guard += icon_.TraceObjectChanges(reason, fn);
   return guard;
}
