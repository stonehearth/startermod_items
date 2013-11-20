#include "pch.h"
#include "om/entity.h"
#include "dm/store.h"
#include "data_store.ridl.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

#if 0
std::ostream& om::operator<<(std::ostream& os, DataStore const& o)
{
   return (os << "[DataStore " << o.GetObjectId() << "]");
}
#endif

void DataStore::ConstructObject()
{
   NOT_YET_IMPLEMENTED(); // Implement save and load please!
   last_encode_ = 0;
}

#if 0
void DataStore::SaveValue(const dm::Store& store, Protocol::Value* msg) const
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

void DataStore::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   cached_json_ = libjson::parse(json);
   last_encode_ = GetLastModified();
   
   try {
      controller_ = lua::ScriptHost::JsonToLua(GetStore().GetInterpreter(), cached_json_);
   } catch (std::exception& e) {
      LOG(WARNING) << "fatal exception loading DataStore: " << e.what();
   }
}

void DataStore::GetDbgInfo(dm::DbgInfo &info) const
{
   if (WriteDbgInfoHeader(info)) {
   }
}

#endif

JSONNode DataStore::GetJsonData() const
{
   using namespace luabind;
   dm::GenerationId last_modified = GetLastModified();
   if (last_encode_ < last_modified) {
      cached_json_ = JSONNode();
      luabind::object c = *controller_;
      if (c.is_valid() && type(c) != LUA_TNIL) {
         cached_json_ = lua::ScriptHost::LuaToJson(c.interpreter(), c);
      }
      last_encode_ = last_modified;
   }
   return cached_json_;
}
