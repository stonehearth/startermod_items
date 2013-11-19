#include "pch.h"
#include "lua_components.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void LuaComponents::ExtendObject(json::Node const& obj) 
{
}

DataBindingPtr LuaComponents::GetLuaComponent(std::string name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? nullptr : i->second;
}

DataBindingPtr LuaComponents::AddLuaComponent(std::string name)
{
   auto i = lua_components_.find(name);
   if (i != lua_components_.end()) {
      return i->second;
   }
   auto component = GetStore().AllocObject<DataBinding>();
   lua_components_.Insert(name, component);
   return component;
}
