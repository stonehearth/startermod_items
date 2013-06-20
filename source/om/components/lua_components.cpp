#include "pch.h"
#include "lua_components.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

#if 0
static luabind::object CastObject(dm::ObjectPtr obj)
{
   luabind::object result;
   if (obj) {
      switch (obj->GetObjectType()) {
      }
   }
   return result;
}

dm::ObjectPtr LuaStoreTable::Get(std::string const& name)
{
   auto i = find(name);
   if (i != end()) {
      return GetStore().FetchObject<dm::Object>(i->second);
   }
   return nullptr;
}

luabind::object LuaStoreTable::GetLua(std::string const& name) {
   return CastObject(Get(name));
}

   
template <class T> std::shared_ptr<T> LuaStoreTable::Get(std::string const& name) const {
   auto obj = Get(name);
   if (obj && obj->GetType() == T::DmObjectType) {
      return std::static_cast_pointer_cast<T>(obj);
   }
   return nullptr;
}

void LuaStoreTable::Set(std::string const& name, dm::ObjectPtr obj)
{
}

void LuaStoreTable::SetLua(std::string const& name, luabind::object value)
{
   switch (luabind::type(value)) {
   case LUA_TSTRING:
      
   default:
      ASSERT(false);
   }
}
#endif

void LuaComponent::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;
   module(L) [
      class_<LuaComponent, LuaComponentRef>("LuaComponent")
         .def(tostring(self))
         .def("save_data",      &om::LuaComponent::SaveJsonData)
   ];
}

std::string LuaComponent::ToString() const
{
   std::ostringstream o;
   o << "[LuaComponent " << GetObjectId() << "]";
   return o.str();
}

void LuaComponent::SaveJsonData(std::string const& data)
{
   json_ = libjson::parse(data);
   MarkChanged();
}

void LuaComponents::ExtendObject(json::ConstJsonObject const& obj) 
{
}

luabind::scope LuaComponents::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   LuaComponent::RegisterLuaType(L);
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

LuaComponentPtr LuaComponents::GetLuaComponent(const char* name) const
{
   auto i = lua_components_.find(name);
   return i == lua_components_.end() ? nullptr : i->second;
}

LuaComponentPtr LuaComponents::AddLuaComponent(const char* name)
{
   ASSERT(lua_components_.find(name) == lua_components_.end());

   auto component = GetStore().AllocObject<LuaComponent>();
   lua_components_[name] = component;
   return component;
}

