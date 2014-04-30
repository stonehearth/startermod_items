#ifndef _RADIANT_OM_MACROS_H
#define _RADIANT_OM_MACROS_H

#include "om/object_enums.h"

#define DEFINE_OM_OBJECT_TYPE_NO_CONS(Class, lower)   \
   enum { DmType = Class ## ObjectType }; \
   static dm::ObjectType GetObjectTypeStatic() { return Class::DmType; } \
   dm::ObjectType GetObjectType() const override { return Class::DmType; } \
   const char *GetObjectClassNameLower() const override { return #lower; } \
   static const char *GetClassNameLower() { return #lower; }


#define DEFINE_OM_OBJECT_TYPE(Class, lower)   \
   DEFINE_OM_OBJECT_TYPE_NO_CONS(Class, lower) \
   Class() { } \
   ~Class() { }


#endif //  _RADIANT_OM_MACROS_H
