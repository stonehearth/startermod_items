#ifndef _RADIANT_OM_ALL_OBJECT_TYPES_H
#define _RADIANT_OM_ALL_OBJECT_TYPES_H

#include <memory>
#include "namespace.h"
#include "dm/dm.h"
#include "all_objects.h"

BEGIN_RADIANT_OM_NAMESPACE

#define OM_OBJECT(Cls, lower)    Cls ## ObjectType,

enum OmObjectTypes {
   OmObjectTypeBase = 1000,
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS
   OmObjectTypeLast
};

#undef OM_OBJECT

std::string GetObjectName(dm::ObjectPtr obj);
std::string GetObjectName(const dm::Object& obj);
std::string GetObjectNameLower(dm::ObjectPtr obj);
std::string GetObjectNameLower(const dm::Object& obj);

#define DEFINE_OM_OBJECT_TYPE(Class)   \
   enum { DmType = Class ## ObjectType }; \
   dm::ObjectType GetObjectType() const override { return Class::DmType; } \
   static const char *GetObjectClassName() { return #Class; } \

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ALL_OBJECT_TYPES_H
