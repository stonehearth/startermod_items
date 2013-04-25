#ifndef _RADIANT_RESOURCES_REGION_2D_H
#define _RADIANT_RESOURCES_REGION_2D_H

#include "model.h"
#include "resource.h"
#include "csg/region.h"
#include <unordered_map>

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Region2d : public Resource {
   public:
      static ResourceType Type;
      ResourceType GetType() const override { return Resource::REGION_2D; }
      const csg::Region2& GetRegion() const { return region_; }
      csg::Region2& ModifyRegion() { return region_; }

   protected:
      csg::Region2      region_;
};

std::ostream& operator<<(std::ostream& out, const Region2d& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_REGION_2D_H
 