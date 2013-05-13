#ifndef _RADIANT_RESOURCES_RESOURCE_H
#define _RADIANT_RESOURCES_RESOURCE_H

#include <memory>
#include <ostream>
#include "namespace.h"
#include "csg/region.h"
#include "libjson.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Resource {
public:
   enum ResourceType {
      ANIMATION,
      JSON,
   };
   virtual ResourceType GetType() const = 0;

private:
};

csg::Region2 ParsePortal(JSONNode const& obj);

typedef std::shared_ptr<Resource> ResourcePtr;

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_H
