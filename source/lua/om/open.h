#ifndef _RADIANT_LUA_OM_OPEN_H
#define _RADIANT_LUA_OM_OPEN_H

#include "../namespace.h"
#include "dm/dm.h"

BEGIN_RADIANT_LUA_NAMESPACE

namespace om {
   void register_json_to_lua_objects(lua_State* L, dm::Store& dm);
   void open(lua_State* lua);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_OM_OPEN_H