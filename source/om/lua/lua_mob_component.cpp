#include "pch.h"
#include "lua/register.h"
#include "lua_mob_component.h"
#include "om/components/mob.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

auto Mob_TraceTransform(Mob const& mob, const char* reason) -> decltype(mob.GetBoxedTransform().CreatePromise(reason))
{
   return mob.GetBoxedTransform().CreatePromise(reason);
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
      lua::RegisterDerivedObject<Mob, Component>()
         .def("get_location",                &Mob::GetLocation)
         .def("extend",                      &Mob::ExtendObject)
         .def("get_grid_location",           &Mob::GetGridLocation)
         .def("get_world_grid_location",     &Mob::GetWorldGridLocation)
         .def("get_world_location",          &Mob::GetWorldLocation)
         .def("set_location",                &Mob::MoveTo)
         .def("set_location_grid_aligned",   &Mob::MoveToGridAligned)
         .def("turn_to",                     &Mob::TurnToAngle)
         .def("turn_to_face_point",          &Mob::TurnToFacePoint)
         .def("set_interpolate_movement",    &Mob::SetInterpolateMovement)
         .def("trace_transform",             &Mob_TraceTransform)
      ,
      dm::Boxed<csg::Transform>::RegisterLuaType(L)
      ;
}
