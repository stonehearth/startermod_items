#ifndef _RADIANT_OM_ALL_OBJECT_TYPES_H
#define _RADIANT_OM_ALL_OBJECT_TYPES_H

#include <memory>
#include "namespace.h"
#include "all_object_defs.h"
#include "all_component_defs.h"

BEGIN_RADIANT_OM_NAMESPACE

#define OM_OBJECT(Cls, lower)    Cls ## ObjectType,

enum OmObjectTypes {
   OmObjectTypeBase = 1000,
   
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS
   BoxedRegion3ObjectType,
   OmObjectTypeLast
};

#undef OM_OBJECT

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ALL_OBJECT_TYPES_H
