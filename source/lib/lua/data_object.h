#ifndef _RADIANT_LUA_DATA_OBJECT_H
#define _RADIANT_LUA_DATA_OBJECT_H

#include "bind.h"
#include "dm/dm.h"
#include "lib/json/node.h"
#include "script_host.h"

BEGIN_RADIANT_LUA_NAMESPACE

class DataObject
{
public:
   DataObject() :
      dirty_(false)
   {
   }

   DataObject(luabind::object o) :
      data_object_(o),
      dirty_(true)
   {
   }

   luabind::object GetDataObject() const
   {
      return data_object_;
   }

   json::Node const& GetJsonNode() const
   {
      if (dirty_) {
         if (data_object_.is_valid() && luabind::type(data_object_) != LUA_TNIL) {
            cached_json_ = ScriptHost::LuaToJson(data_object_.interpreter(), data_object_);
         } else {
            cached_json_ = JSONNode();
         }
         dirty_ = false;
      }
      return cached_json_;
   }

   void SetJsonNode(json::Node const& node)
   {
      ASSERT(!data_object_.is_valid()); // should only be used on the client by Load()...
      dirty_ = false;
      cached_json_ = node;
   }

public:
   luabind::object            data_object_;
   mutable json::Node         cached_json_;
   mutable dm::GenerationId   dirty_;
};

END_RADIANT_LUA_NAMESPACE

BEGIN_RADIANT_NAMESPACE

template<>
struct dm::SaveImpl<lua::DataObject> {
   static void SaveValue(const dm::Store& store, Protocol::Value* msg, lua::DataObject const& obj) {
      SaveImpl<json::Node>::SaveValue(store, msg, obj.GetJsonNode());
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, lua::DataObject& obj) {
      json::Node node;
      SaveImpl<json::Node>::LoadValue(store, msg, node);
      obj.SetJsonNode(node);
   }
   static void GetDbgInfo(lua::DataObject const& obj, dm::DbgInfo &info) {
      info.os << "[data_object " << obj.GetJsonNode().write() << "]";
   }
};

END_RADIANT_NAMESPACE

#endif
