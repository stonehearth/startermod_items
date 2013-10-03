#include "pch.h"
#include "json_store.h"
#include "om/entity.h"
#include "dm/store.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const JsonStore& o)
{
   return (os << "[JsonStore " << o.GetObjectId() << "]");
}

JSONNode& JsonStore::Modify()
{
   MarkChanged();
   return json_;
}

JSONNode const& JsonStore::Get() const
{
   return json_;
}


void JsonStore::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   std::string json = json_.write();
   dm::SaveImpl<std::string>::SaveValue(store, msg, json);
}

void JsonStore::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   json_ = libjson::parse(json);
}

void JsonStore::GetDbgInfo(dm::DbgInfo &info) const
{
   if (WriteDbgInfoHeader(info)) {
   }
}
