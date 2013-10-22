#include "pch.h"
#include "lib/lua/register.h"
#include "lua_render_info_component.h"
#include "om/components/render_info.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

template <typename T, typename Cls, dm::Boxed<T> const& (Cls::*Fn)() const>
T Lua_AttributeGetter(std::weak_ptr<Cls> const w)
{
   auto p = w.lock();
   if (p)  {
      return *(((*p).*Fn)());
   }
   throw std::invalid_argument("invalid reference in native getter");
   return T(); // unreached
}

template <typename T, typename Cls, void (Cls::*Fn)(T const&)>
std::weak_ptr<Cls> Lua_AttributeSetter(std::weak_ptr<Cls> w, T const& value)
{
   auto p = w.lock();
   if (p) {
      (((*p).*Fn)(value));
   }
   return w;
}

template <typename T, typename Cls, typename A0, T (Cls::*Fn)(A0)>
T Lua_CallMethod_1(std::weak_ptr<Cls> w, A0 arg0)
{
   auto p = w.lock();
   if (p) {
      return (((*p).*Fn)(arg0));
   }
   throw std::invalid_argument("invalid reference in native getter");
   return T(); // unreached
}


scope LuaRenderInfoComponent::RegisterLuaTypes(lua_State* L)
{
#define LUA_SETTER(lua_name, T, Cls, Method)    .def("set_" lua_name, &Lua_AttributeSetter<T, Cls, &Cls::Set ## Method>)
#define LUA_GETTER(lua_name, T, Cls, Method)    .def("get_" lua_name, &Lua_AttributeGetter<T, Cls, &Cls::Get ## Method>)
#define LUA_ATTRIBUTE(lua_name, T, Cls, Method) \
   LUA_SETTER(lua_name, T, Cls, Method)         \
   LUA_GETTER(lua_name, T, Cls, Method)         \

#define LUA_METHOD_1(lua_name, T, Cls, Method, A0)    .def(lua_name, &Lua_CallMethod_1<T, Cls, A0, &Cls::Method>)

   return
      lua::RegisterWeakGameObjectDerived<RenderInfo, Component>()
         LUA_ATTRIBUTE("model_variant",   std::string, RenderInfo, ModelVariant)
         LUA_ATTRIBUTE("material",        std::string, RenderInfo, Material)
         LUA_ATTRIBUTE("animation_table", std::string, RenderInfo, AnimationTable)
         LUA_METHOD_1("attach_entity", void,      RenderInfo, AttachEntity, EntityRef)
         LUA_METHOD_1("remove_entity", EntityRef, RenderInfo, RemoveEntity, std::string const&)
      ;
}
