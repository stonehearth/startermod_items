#include "pch.h"
#include "dm/store.h"
#include "data_object.h"
#include "protocols/store.pb.h"
#include "lib/marshall/convert.h"

using namespace radiant;
using namespace radiant::lua;

#define DO_LOG(level)      LOG_CATEGORY(lua.data, level, " v: " << data_object_.is_valid() << " i: " << (data_object_.interpreter()) << " h:" << GetLuaObjectIndex(&data_object_))

// This is super dirty.  We know the index is 4 bytes into the
// object (even though it's private...), so just grab it.  This
// should only be used for debug logging!
static inline int GetLuaObjectIndex(luabind::object const* o)
{   
   return ((int *)o)[1];
}

DataObject::DataObject() :
   dirty_(false)
{
   DO_LOG(8) << "data object default constructor";
}

DataObject::DataObject(luabind::object o) :
   dirty_(true)
{
   DO_LOG(8) << "data object constructor";
   SetDataObject(o);
}

void DataObject::SetDataObject(luabind::object o) 
{
   DO_LOG(8) << "set data object to (new object: " << GetLuaObjectIndex(&o) << ")";
   // make sure we use a durable interpreter!
   lua_State* L = ScriptHost::GetInterpreter(o.interpreter());
   data_object_ = luabind::object(L, o);
   DO_LOG(8) << "finished set data object (old object: " << GetLuaObjectIndex(&o) << ")";

   MarkDirty();
}

luabind::object DataObject::GetDataObject() const
{
   DO_LOG(8) << "get data object";
   return data_object_;
}

void DataObject::MarkDirty()
{
   // xxx: look for cycles!
   DO_LOG(8) << "mark dirty";
   dirty_ = true;
}

json::Node const& DataObject::GetJsonNode() const
{
   if (dirty_) {
      DO_LOG(8) << "recomputing json node";
      if (data_object_.is_valid() && luabind::type(data_object_) != LUA_TNIL) {
         cached_json_ = ScriptHost::LuaToJson(data_object_.interpreter(), data_object_);         
      } else {
         cached_json_ = JSONNode();
      }
      dirty_ = false;
   }
   return cached_json_;
}

#if 0
void DataObject::SetJsonNode(lua_State* L, json::Node const& node)
{
   DO_LOG(8) << "set json node";
   dirty_ = false;
   cached_json_ = node;
   data_object_ = ScriptHost::JsonToLua(L, node);
   DO_LOG(8) << "post set json node";
}
#endif

void DataObject::SaveValue(dm::Store const& store, dm::SerializationType r, Protocol::LuaDataObject *msg) const
{
   lua::ScriptHost* s = lua::ScriptHost::GetScriptHost(store.GetInterpreter());
   ASSERT(s);
   if (s) {
      int flags;
      if (r == dm::REMOTING) { flags |= marshall::Convert::REMOTE; }
      marshall::Convert(store, flags).ToProtobuf(data_object_, msg->mutable_lua_object());
   }
}

void DataObject::LoadValue(dm::Store const& store, dm::SerializationType  r, const Protocol::LuaDataObject &msg)
{
   lua::ScriptHost* s = lua::ScriptHost::GetScriptHost(store.GetInterpreter());
   ASSERT(s);
   if (s) {
      int flags;
      if (r == dm::REMOTING) { flags |= marshall::Convert::REMOTE; }
      marshall::Convert(store, flags).ToLua(msg.lua_object(), data_object_);
      dirty_ = true;
   }
}

