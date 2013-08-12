#include "pch.h"
#include "lua/register.h"
#include "lua_entity.h"
#include "om/all_components.h"
#include "om/entity.h"
#include "resources/res_manager.h"
#include "simulation/script/script_host.h" // xxx: NOOOOOOOOOOOOO! Use a generic script host!!!!!!!!

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

static luabind::object
Entity_GetNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         return luabind::object(); \
      } \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}


static std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      JSONNode const& manifest = resources::ResourceManager2::GetInstance().LookupManifest(modname);
      return manifest["components"][name].as_string();
   }
   // xxx: throw an exception...
   return "";
}

static luabind::object
Entity_GetLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   om::LuaComponentsPtr component = entity->GetComponent<om::LuaComponents>();
   if (component) {
      om::LuaComponentPtr lua_component = component->GetLuaComponent(name);
      if (lua_component) {
         return lua_component->GetLuaObject();
      }
   }
   return luabind::object();
}

static luabind::object
Entity_GetComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = Entity_GetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = Entity_GetLuaComponent(L, entity, name);
      }
   }
   return component;
}


static luabind::object
Entity_AddNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->AddComponent<om::Clas>(); \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

static luabind::object
Entity_AddLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   using namespace luabind;

   object result;
   om::LuaComponentsPtr component = entity->AddComponent<om::LuaComponents>();
   om::LuaComponentPtr lua_component = component->GetLuaComponent(name);
   if (lua_component) {
      result = lua_component->GetLuaObject();
   } else {
      std::string uri = GetLuaComponentUri(name);
      simulation::ScriptHost* host = luabind::object_cast<simulation::ScriptHost*>(globals(L)["native"]);
      ASSERT(host);
      object ctor = host->LuaRequire(uri);
      if (ctor) {
         lua_component = component->AddLuaComponent(name);         
         result = call_function<object>(ctor, om::EntityRef(entity), om::LuaComponentRef(lua_component));
         lua_component->SetLuaObject(name, result);
      }
   }
   return result;
}

static luabind::object
Entity_AddComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = Entity_AddNativeComponent(L, entity, name);

      if (!component.is_valid()) {
         component = Entity_AddLuaComponent(L, entity, name);
         ASSERT(component.is_valid());
      }
   }
   return component;
}

scope LuaEntity::RegisterLuaTypes(lua_State* L)
{
   return
      //class_<Entity, std::weak_ptr<Entity>>(name)
      lua::RegisterObject<Entity>()
         .def("get_debug_name",     &Entity::GetDebugName)
         .def("set_debug_name",     &Entity::SetDebugName)
         .def("get_resource_uri",   &Entity::GetResourceUri)
         .def("set_resource_uri",   &Entity::SetResourceUri)
         .def("get_component" ,     &Entity_GetComponent)
         .def("add_component" ,     &Entity_AddComponent)
      ;
}

