#ifndef _RADIANT_RESOURCES_SKELETON_H
#define _RADIANT_RESOURCES_SKELETON_H

#include "resource.h"
#include "libjson.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Skeleton : public Resource {
   public:
      static ResourceType Type;
      Skeleton(const JSONNode& info) : info_(info) { }
      
      ResourceType GetType() const override { return Resource::SKELETON; }
      const JSONNode& GetJson() const { return info_; }

   protected:
      JSONNode          info_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_SKELETON_H
 