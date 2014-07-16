#ifndef _RADIANT_DM_LUA_TYPES_H
#define _RADIANT_DM_LUA_TYPES_H

#include "dm/map.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_DM_NAMESPACE

class NumberMap : public dm::Map<int, luabind::object>
{
public:
   typedef dm::Map<int, luabind::object> MapType;

public:
   static ObjectType GetObjectTypeStatic() { return NumberMapObjectType; }
   ObjectType GetObjectType() const override { return NumberMapObjectType; }
   const char *GetObjectClassNameLower() const override { return "numbermap"; }
};

DECLARE_SHARED_POINTER_TYPES(NumberMap)

END_RADIANT_DM_NAMESPACE

#endif
