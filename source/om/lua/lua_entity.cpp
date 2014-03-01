#include "pch.h"
#include "lib/lua/register.h"
#include "lib/lua/dm/trace_wrapper.h"
#include "lua_entity.h"
#include "om/all_components.h"
#include "om/entity.h"
#include "om/components/data_store.ridl.h"
#include "om/stonehearth.h"
#include "dm/trace_categories.h"
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

lua::TraceWrapperPtr Entity_TraceObject(std::weak_ptr<Entity> e, const char *reason)
{
   auto entity = e.lock();
   if (entity) {
      return std::make_shared<lua::TraceWrapper>(entity->TraceObjectChanges(reason, dm::LUA_ASYNC_TRACES));
   }
   throw std::exception("cannot trace expired entity reference");
}

scope LuaEntity::RegisterLuaTypes(lua_State* L)
{
   return
      //class_<Entity, std::weak_ptr<Entity>>(name)
      lua::RegisterWeakGameObject<Entity>(L)
         .def("trace_object",       &Entity_TraceObject)
         .def("get_uri",            &Entity_GetUri)
         .def("get_debug_text",     &Entity::GetDebugText)
         .def("set_debug_text",     &Entity::SetDebugText)
         .def("get_component" ,     &om::Stonehearth::GetComponent)
         .def("add_component" ,     &om::Stonehearth::AddComponent)
         .def("add_component" ,     &om::Stonehearth::SetComponentData)
         .def("get_component_data", &om::Stonehearth::GetComponentData)
         .def("set_component_data", &om::Stonehearth::SetComponentData)
      ;
}

