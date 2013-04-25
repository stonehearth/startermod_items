#ifndef _RADIANT_SIMULATION_LUA_RESOURCE_H
#define _RADIANT_SIMULATION_LUA_RESOURCE_H

#include "namespace.h"
#include "luabind/luabind.hpp"
#include "resources/object_resource.h"
#include "resources/array_resource.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Action;

class LuaResource
{
public:
   static void RegisterType(lua_State* L);
   static luabind::object ConvertResourceToLua(lua_State* L, std::shared_ptr<resources::Resource> obj);
};

class LuaObjectResource
{
public:
   LuaObjectResource(const resources::ObjectResource* obj);
   
private:
   friend LuaResource;
   void StartIteration(lua_State *L);
   luabind::object LookupItem(lua_State *L, luabind::object arg0, luabind::object deflt);

private:
   const resources::ObjectResource* obj_;
};

class LuaObjectResourceIterator
{
public:
   LuaObjectResourceIterator(const resources::ObjectResource* obj);

private:
   friend LuaResource;
   void NextIteration(lua_State *L, luabind::object s, luabind::object var);

private:
   const resources::ObjectResource* obj_;
   resources::ObjectResource::ResourceMap::const_iterator iter_;
};

class LuaArrayResourceIterator
{
public:
   LuaArrayResourceIterator(const resources::ArrayResource& obj);

private:
   friend LuaResource;
   void NextIteration(lua_State *L, luabind::object s, luabind::object var);

private:
   const resources::ArrayResource& obj_;
   int                             i_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_LUA_RESOURCE_H