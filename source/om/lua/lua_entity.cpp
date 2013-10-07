#include "pch.h"
#include "lua/register.h"
#include "lua_entity.h"
#include "om/all_components.h"
#include "om/entity.h"
#include "om/data_binding.h"
#include "om/stonehearth.h"
#include "resources/res_manager.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

std::string Entity_GetUri(std::weak_ptr<Entity> e)
{
   auto entity = e.lock();
   if (entity) {
      return entity->GetUri();
   }
   return "";
}

scope LuaEntity::RegisterLuaTypes(lua_State* L)
{
   return
      //class_<Entity, std::weak_ptr<Entity>>(name)
      lua::RegisterObject<Entity>()
         .def("get_uri",            &Entity_GetUri)
         .def("get_debug_text",     &Entity::GetDebugText)
         .def("set_debug_text",     &Entity::SetDebugText)
         .def("get_component" ,     &om::Stonehearth::GetComponent)
         .def("add_component" ,     &om::Stonehearth::AddComponent)
      ;
}

