#ifndef _RADIANT_DM_ALL_OBJECT_TYPES_H
#define _RADIANT_DM_ALL_OBJECT_TYPES_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

#define ALL_DM_OBJECTS \
   DM_OBJECT(Boxed) \
   DM_OBJECT(Set) \
   DM_OBJECT(Map) \
   DM_OBJECT(Record) \
   DM_OBJECT(Array) \
   DM_OBJECT(Queue) \

#define DM_OBJECT(Obj)     Obj ## ObjectType,

enum {
   DmObjectTypeBase = 1,
   ALL_DM_OBJECTS
};

#undef DM_OBJECT

#define DEFINE_DM_OBJECT_TYPE(Class, lower)   \
   enum { DmType = Class ## ObjectType }; \
   ObjectType GetObjectType() const override { return Class::DmType; } \
   const char *GetObjectClassNameLower() const override { return #lower; }

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_ALL_OBJECT_TYPES_H
