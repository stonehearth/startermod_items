#ifndef _RADIANT_RESOURCES_DATA_RESOURCE_H
#define _RADIANT_RESOURCES_DATA_RESOURCE_H

#include "resource.h"
#include "libjson.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class DataResource : public Resource {
   public:
      DataResource(ResourceType type, const JSONNode& info) : type_(type), info_(info) { }
      
      ResourceType GetType() const override { return type_; }
      const JSONNode& GetJson() const { return info_; }
      std::string GetJsonString() const { return info_.write(); }

   protected:
      ResourceType      type_;
      JSONNode          info_;
};

typedef std::shared_ptr<DataResource>  DataResourcePtr;
std::ostream& operator<<(std::ostream& out, const DataResource& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_DATA_RESOURCE_H
