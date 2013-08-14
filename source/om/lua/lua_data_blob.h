#ifndef _RADIANT_OM_LUA_DATA_BLOB_H
#define _RADIANT_OM_LUA_DATA_BLOB_H

#include "radiant_luabind.h"
#include "om/namespace.h"

BEGIN_RADIANT_OM_NAMESPACE

struct LuaDataBlob {
   static luabind::scope RegisterLuaTypes(lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
