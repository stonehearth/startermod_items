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
   DataObject();
   DataObject(luabind::object o);

   void SetDataObject(luabind::object o);
   luabind::object GetDataObject() const;
   void MarkDirty();

   json::Node const& GetJsonNode() const;
#if 0
   void SetJsonNode(lua_State* L, json::Node const& node);
#endif

   void SaveValue(dm::Store const& store, dm::SerializationType r, Protocol::LuaDataObject *msg) const;
   void LoadValue(dm::Store const& store, dm::SerializationType r, const Protocol::LuaDataObject &msg);

public:
   luabind::object            data_object_;
   mutable json::Node         cached_json_;
   mutable dm::GenerationId   dirty_;
};

DECLARE_SHARED_POINTER_TYPES(DataObject)

END_RADIANT_LUA_NAMESPACE

#endif
