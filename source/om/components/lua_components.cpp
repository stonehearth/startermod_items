#include "pch.h"
#include "lua_components.h"
#include "data_store.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, LuaComponents const& o)
{
   return (os << "[LuaComponents]");
}

void LuaComponents::ExtendObject(json::Node const& obj) 
{
}

DataStorePtr LuaComponents::GetLuaComponent(std::string name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? nullptr : i->second;
}

DataStorePtr LuaComponents::AddLuaComponent(std::string name)
{
   auto i = lua_components_.find(name);
   if (i != lua_components_.end()) {
      return i->second;
   }
   auto component = GetStore().AllocObject<DataStore>();
   lua_components_.Insert(name, component);
   return component;
}
