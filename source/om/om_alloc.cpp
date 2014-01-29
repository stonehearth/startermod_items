#include "pch.h"
#include <memory>
#include "om_alloc.h"

using namespace ::radiant;
using namespace ::radiant::om;

void radiant::om::RegisterObjectTypes(dm::Store& store)
{
#  define OM_OBJECT(Clas, lower) \
   store.RegisterAllocator(Clas ## ObjectType, []() -> std::shared_ptr<Clas> { \
      return std::make_shared<Clas>(); \
   });

      OM_ALL_OBJECTS
      OM_ALL_COMPONENTS
      OM_OBJECT(Region3Boxed, boxed_region3)
      OM_OBJECT(JsonBoxed, boxed_json)

#  undef OM_OBJECT
}
