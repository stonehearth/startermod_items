#include "pch.h"
#include "data_binding.h"
#include "om/entity.h"
#include "dm/store.h"
#include "lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, DataBinding const& o)
{
   return (os << "[DataBinding " << o.GetObjectId() << "]");
}

DataBinding::DataBinding() :
   dm::Object(),
   last_encode_(0)
{
}

void DataBinding::SetDataObject(luabind::object data)
{
   data_ = data;
   MarkChanged();
}

luabind::object DataBinding::GetDataObject() const
{
   return data_;
}

void DataBinding::SetModelObject(luabind::object model)
{
   model_ = model;
}

luabind::object DataBinding::GetModelObject() const
{
   return model_;
}

void DataBinding::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   // xxx: this isn't going to work for save and load.  we need to serialize something
   // which can then be unserialized (probably via eval!!), which means it will have
   // class names in it, which means it won't be valid json! 
   //
   // this is idea for remoting, though, so let's do it.  SaveValue/LoadValue should
   // probably be renamed to something which indicates they're for remoting (or
   // removed entirely from the object and moved elsewhere!!!)
   std::string json = GetJsonData().write();
   dm::SaveImpl<std::string>::SaveValue(store, msg, json);
}

void DataBinding::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   cached_json_ = libjson::parse(json);
   last_encode_ = GetLastModified();
   
   try {
      data_ = lua::ScriptHost::JsonToLua(GetStore().GetInterpreter(), cached_json_);
   } catch (std::exception& e) {
      LOG(WARNING) << "fatal exception loading DataBinding: " << e.what();
   }
}

JSONNode DataBinding::GetJsonData() const
{
   using namespace luabind;
   dm::GenerationId last_modified = GetLastModified();
   if (last_encode_ < last_modified) {
      cached_json_ = JSONNode();      
      if (data_.is_valid() && type(data_) != LUA_TNIL) {
         cached_json_ = lua::ScriptHost::LuaToJson(data_.interpreter(), data_);
      }
      last_encode_ = last_modified;
   }
   return cached_json_;
}

void DataBinding::GetDbgInfo(dm::DbgInfo &info) const
{
   if (WriteDbgInfoHeader(info)) {
   }
}
