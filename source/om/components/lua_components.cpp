#include "pch.h"
#include "lua_components.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void LuaComponents::InitializeRecordFields()
{
   Component::InitializeRecordFields();
}

void LuaComponents::ExtendObject(json::ConstJsonObject const& obj) 
{
}

luabind::scope LuaComponents::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<LuaComponents, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("get_lua_component",               &om::LuaComponents::GetLuaComponent)
         .def("add_lua_component",               &om::LuaComponents::AddLuaComponent)
      ;
}

std::string LuaComponents::ToString() const
{
   std::ostringstream os;
   os << "(LuaComponents id:" << GetObjectId() << ")";
   return os.str();
}

luabind::object LuaComponents::GetLuaComponent(const char* name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? luabind::object() : i->second;
}

void LuaComponents::AddLuaComponent(const char* name, luabind::object api)
{
   ASSERT(!GetLuaComponent(name).is_valid());
   lua_components_[name] = api;
}

