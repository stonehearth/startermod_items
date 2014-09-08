#include "radiant.h"
#include <memory>
#include "om_alloc.h"

#define DEFINE_ALL_OBJECTS
#include "all_objects.h"
#include "all_components.h"

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
      OM_OBJECT(Region2Boxed,  boxed_region2)
      OM_OBJECT(Region3Boxed,  boxed_region3)
      OM_OBJECT(Region2fBoxed, boxed_region2f)
      OM_OBJECT(Region3fBoxed, boxed_region3f)
      OM_OBJECT(JsonBoxed, boxed_json)

#  undef OM_OBJECT
}
