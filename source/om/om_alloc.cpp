#include "pch.h"
#include <memory>
#include "om_alloc.h"

using namespace ::radiant;
using namespace ::radiant::om;

      //LOG(WARNING) << "creating new " #Clas " component"; \

void radiant::om::RegisterObjectTypes(dm::Store& store)
{
#  define OM_OBJECT(Clas, lower) \
   store.RegisterAllocator(Clas ## ObjectType, []() -> std::shared_ptr<Clas> { \
      return std::make_shared<Clas>(); \
   });

      OM_ALL_OBJECTS
      OM_ALL_COMPONENTS

#  undef OM_OBJECT
}
