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
   json::Node const& GetJsonNode() const;
   void MarkDirty();

   bool GetNeedsRestoration() const;
   void SetNeedsRestoration(bool value) const;

   void SaveValue(dm::Store const& store, dm::SerializationType r, Protocol::LuaDataObject *msg) const;
   void LoadValue(dm::Store const& store, dm::SerializationType r, const Protocol::LuaDataObject &msg);

public:
   luabind::object            data_object_;
   mutable json::Node         cached_json_;
   mutable dm::GenerationId   dirty_;
   mutable bool               needsRestoration_;
};

DECLARE_SHARED_POINTER_TYPES(DataObject)

END_RADIANT_LUA_NAMESPACE

#endif
