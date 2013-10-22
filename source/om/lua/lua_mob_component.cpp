#include "pch.h"
#include "lib/lua/register.h"
#include "lib/lua/script_host.h"
#include "lua_mob_component.h"
#include "om/components/mob.h"
#include "lib/json/core_json.h"
#include "lib/json/csg_json.h"
#include "lib/json/dm_json.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

auto Mob_TraceTransform(Mob const& mob, const char* reason) -> decltype(mob.GetBoxedTransform().CreatePromise(reason))
{
   return mob.GetBoxedTransform().CreatePromise(reason);
}

void Mob_ExtendObject(lua_State* L, Mob& mob, luabind::object o)
{
   mob.ExtendObject(lua::ScriptHost::LuaToJson(L, o));
}

om::EntityRef Mob_GetParent(Mob const& mob)
{
   auto parent = mob.GetParent();
   if (parent) {
      return parent->GetEntityRef();
   }
   return om::EntityRef();
}

std::ostream& operator<<(std::ostream& os, dm::Boxed<csg::Transform> const& f)
{
   return os << "[BoxedTransform]";
}

std::ostream& operator<<(std::ostream& os, dm::Boxed<csg::Transform>::Promise const& f)
{
   return os << "[BoxedTransformPromise]";
}

scope LuaMobComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<Mob, Component>()
         .def("get_location",                &Mob::GetLocation)
         .def("extend",                      &Mob_ExtendObject)
         .def("get_grid_location",           &Mob::GetGridLocation)
         .def("get_world_grid_location",     &Mob::GetWorldGridLocation)
         .def("get_world_location",          &Mob::GetWorldLocation)
         .def("set_location",                &Mob::MoveTo)
         .def("set_location_grid_aligned",   &Mob::MoveToGridAligned)
         .def("turn_to",                     &Mob::TurnToAngle)
         .def("turn_to_face_point",          &Mob::TurnToFacePoint)
         .def("set_interpolate_movement",    &Mob::SetInterpolateMovement)
         .def("get_transform",               &Mob::GetTransform)
         .def("set_transform",               &Mob::SetTransform)
         .def("trace_transform",             &Mob_TraceTransform)
         .def("get_parent",                  &Mob_GetParent)
      ,
      dm::Boxed<csg::Transform>::RegisterLuaType(L)
      ;
}
