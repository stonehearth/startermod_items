#include "pch.h"
#include "om/all_object_types.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

luabind::scope Component::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<Component, std::weak_ptr<Component>>(name)
         .def("get_id",     &om::Component::GetObjectId)
         .def("get_entity", &om::Component::GetEntityRef)
      ;
}

std::string radiant::om::GetObjectNameLower(dm::ObjectPtr obj)
{
   return obj.get() ? GetObjectNameLower(*obj) : std::string("nullptr");
}

std::string radiant::om::GetObjectName(dm::ObjectPtr obj)
{
   return obj.get() ? GetObjectName(*obj) : std::string("nullptr");
}

std::string radiant::om::GetObjectNameLower(const dm::Object& obj)
{
   dm::ObjectType t = obj.GetObjectType();
   static const std::string names[] = {
   #define OM_OBJECT(Cls, lower)    std::string(#lower),
      std::string("om_object_type_base"),
      OM_ALL_OBJECTS
      OM_ALL_COMPONENTS
   #undef OM_OBJECT
   };
   ASSERT(t > OmObjectTypeBase && t < OmObjectTypeLast);
   return names[t - OmObjectTypeBase];
}

std::string radiant::om::GetObjectName(const dm::Object& obj) {
   dm::ObjectType t = obj.GetObjectType();
   static const std::string names[] = {
   #define OM_OBJECT(Cls, lower)    std::string(#Cls),
      std::string("OmObjectTypeBase"),
      OM_ALL_OBJECTS
      OM_ALL_COMPONENTS
   #undef OM_OBJECT
   };
   ASSERT(t > OmObjectTypeBase && t < OmObjectTypeLast);
   return names[t - OmObjectTypeBase];
}

const math3d::ipoint3 Component::GetOrigin() const
{
   auto mob = GetEntity().GetComponent<Mob>();
   return mob ? mob->GetGridLocation() : math3d::ipoint3(0, 0, 0);
}

const math3d::ipoint3 Component::GetWorldOrigin() const
{
   auto mob = GetEntity().GetComponent<Mob>();
   return mob ? mob->GetWorldGridLocation() : math3d::ipoint3(0, 0, 0);
}
