#ifndef _RADIANT_OM_STONEHEARTH_H
#define _RADIANT_OM_STONEHEARTH_H

#include "om.h"
#include "dm/dm.h"
#include "csg/region.h"
#include "lib/json/node.h"

// This file clearly does not belong in the om.

struct lua_State;

BEGIN_RADIANT_OM_NAMESPACE

class Stonehearth
{
public:
   // xxx this file has no reason to exist anymore now that lua handles most of this
   static void InitEntity(om::EntityPtr entity, std::string const& uri, lua_State* L);
   static luabind::object AddComponent(lua_State* L, om::EntityRef e, std::string name);
   static void RemoveComponent(lua_State* L, om::EntityRef e, std::string name);
   static luabind::object GetComponent(lua_State* L, om::EntityRef e, std::string name);
   static luabind::object AddComponentData(lua_State* L, om::EntityRef e, std::string name);
   static luabind::object GetComponentData(lua_State* L, om::EntityRef e, std::string name);
   static luabind::object SetComponentData(lua_State* L, om::EntityRef e, std::string name, luabind::object data);
   static void RestoreLuaComponents(lua::ScriptHost* scriptHost, EntityPtr entity);

private:
   static void InitEntityByUri(om::EntityPtr entity, std::string const& uri, lua_State* L);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_STONEHEARTH_H
