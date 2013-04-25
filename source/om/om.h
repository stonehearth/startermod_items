#ifndef _RADIANT_OM_H
#define _RADIANT_OM_H

#include <memory>
#include "namespace.h"
#include "all_objects.h"
#include "dm/dm.h"

BEGIN_RADIANT_OM_NAMESPACE

typedef int EntityId;

#define DECLARE_INTERFACE(Cls) \
   class Cls;

DECLARE_INTERFACE(CollisionShape)

#undef DECLARE_POINTER_TYPES
#undef DECLARE_INTERFACE

#define OM_OBJECT(Cls, lower) \
   class Cls; \
   typedef std::shared_ptr<Cls>  Cls ## Ptr; \
   typedef std::weak_ptr<Cls>    Cls ## Ref;

OM_ALL_OBJECTS
OM_ALL_COMPONENTS

// abstruct base classes...
   OM_OBJECT(BuildOrder,         build_order)
   OM_OBJECT(GridBuildOrder,     grid_build_order)
   OM_OBJECT(RegionBuildOrder,   region_build_order)

#undef OM_OBJECT

class Selection;
class TileAddress;

void RegisterObjectTypes(dm::Store& store);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
