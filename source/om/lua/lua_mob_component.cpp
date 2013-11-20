#include "pch.h"
#include "lib/lua/register.h"
#include "lib/lua/script_host.h"
#include "lua_mob_component.h"
#include "om/components/mob.ridl.h"
#include "lib/json/core_json.h"
#include "lib/json/csg_json.h"
#include "lib/json/dm_json.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

/*

OM_BEGIN_CLASS(Mob, Component)
   OM_PROPERTY(Mob, EntityRef,        parent);
   OM_PROPERTY(Mob, csg::Transform,   transform);
   OM_PROPERTY(Mob, csg::Cube3f,      aabb);
   OM_PROPERTY(Mob, bool,             interpolate_movement);      
   OM_PROPERTY(Mob, bool,             moving);   

   OM_METHOD(Mob, void, set_location, (csg::Point3f const& location), (location));
   OM_METHOD(Mob, void, set_location_grid_aligned, (csg::Point3 const& location), (location));

   OM_METHOD(Mob, csg::Point3f, get_location, () const, ());
   OM_METHOD(Mob, csg::Point3,  get_grid_location, () const, ());
   OM_METHOD(Mob, csg::Point3f, get_world_location, () const, ());
   OM_METHOD(Mob, csg::Point3,  get_world_grid_location, () const, ());

   OM_METHOD(Mob, void, turn_to, (float angle), (angle));
   OM_METHOD(Mob, void, turn_to_face_point, (csg::Point3 const& location), (location));

public:
   DEFINE_OM_OBJECT_TYPE(Mob, mob);
   void ExtendObject(json::Node const& obj) override;
   virtual void Describe(std::ostringstream& os) const;

private:
   friend EntityContainer;
   void SetParent(MobPtr parent);
   void TurnTo(const csg::Quaternion& orientation);

OM_END_CLASS()

*/

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


dm::Object::LuaPromise<Mob>* Mob_Trace(Mob const& mob, const char* reason)
{
   return new dm::Object::LuaPromise<Mob>(reason, mob);
}

scope LuaMobComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<Mob, Component>()
         .def("extend",                      &Mob_ExtendObject)
         .def("trace_transform",             &Mob_TraceTransform)
         .def("get_parent",                  &Mob_GetParent)
         .def("get_location",                &Mob::GetLocation)
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
         .def("get_aabb",                    &Mob::GetAABB)
         .def("set_aabb",                    &Mob::SetAABB)
         .def("trace",                       &Mob_Trace) // xxx: shouldn't this be generalized in the compnoent?  YES!  YES IT SHOULD!!
      ,
      dm::Boxed<csg::Transform>::RegisterLuaType(L)
      ,
      dm::Object::LuaPromise<Mob>::RegisterLuaType(L)         
      ;
}
