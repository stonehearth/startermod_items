#ifndef _RADIANT_CLIENT_RENDERER_LUA_RENDER_ENTITY_H
#define _RADIANT_CLIENT_RENDERER_LUA_RENDER_ENTITY_H

#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class LuaRenderEntity
{
public:
   static void RegisterType(lua_State* L);
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_LUA_RENDER_ENTITY_H
