#ifndef _RADIANT_RESOURCES_RESOURCE_H
#define _RADIANT_RESOURCES_RESOURCE_H

#include "namespace.h"
#include <memory>
#include <ostream>

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Resource {
public:
   enum ResourceType {
      ARRAY,
      OBJECT,
      NUMBER,
      STRING,
      RIG,
      ANIMATION,
      ACTION,
      REGION_2D,
      SKELETON,
      EFFECT,
      RECIPE,
      ONE_OF,
      JSON,
      COMBAT_ABILITY,
   };
   virtual ResourceType GetType() const = 0;

private:
};

typedef std::shared_ptr<Resource> ResourcePtr;

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_H
