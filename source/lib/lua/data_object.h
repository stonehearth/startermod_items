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

   void SetJsonNode(lua_State* L, json::Node const& node)
   {
      dirty_ = false;
      cached_json_ = node;
      data_object_ = ScriptHost::JsonToLua(L, node);
   }

public:
   luabind::object            data_object_;
   mutable json::Node         cached_json_;
   mutable dm::GenerationId   dirty_;
};

END_RADIANT_LUA_NAMESPACE

#endif
