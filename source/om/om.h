#ifndef _RADIANT_OM_H
#define _RADIANT_OM_H

#include <memory>
#include "namespace.h"
#include "all_object_defs.h"
#include "all_component_defs.h"
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

#undef OM_OBJECT

class Component;
class Selection;
class TileAddress;

DECLARE_SHARED_POINTER_TYPES(Component)

std::string GetObjectName(dm::ObjectPtr obj);
std::string GetObjectName(const dm::Object& obj);
std::string GetObjectNameLower(dm::ObjectPtr obj);
std::string GetObjectNameLower(const dm::Object& obj);


void RegisterObjectTypes(dm::Store& store);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
