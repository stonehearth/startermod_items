#include "pch.h"
#include "component.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const std::string names_lower__[] = {
#define OM_OBJECT(Cls, lower)    std::string(#lower),
   std::string("om_object_type_base"),
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS
   std::string("boxed_region2_object_type"),
   std::string("boxed_region3_object_type"),
   std::string("boxed_json"),
#undef OM_OBJECT
};

static const std::string names_upper__[] = {
#define OM_OBJECT(Cls, lower)    std::string(#Cls),
   std::string("OmObjectTypeBase"),
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS
   std::string("BoxedRegion2ObjectType"),
   std::string("BoxedRegion3ObjectType"),
   std::string("BoxedJson"),
#undef OM_OBJECT
};

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
   ASSERT(t > OmObjectTypeBase && t < OmObjectTypeLast);
   return names_lower__[t - OmObjectTypeBase];
}

std::string radiant::om::GetObjectName(const dm::Object& obj) {
   dm::ObjectType t = obj.GetObjectType();
   ASSERT(t > OmObjectTypeBase && t < OmObjectTypeLast);
   return names_upper__[t - OmObjectTypeBase];
}
