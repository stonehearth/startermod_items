#ifndef _RADIANT_RESOURCES_STRING_RESOURCE_H
#define _RADIANT_RESOURCES_STRING_RESOURCE_H

#include "resource.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class StringResource : public Resource {
public: 
   static ResourceType Type;
   ResourceType GetType() const override { return Resource::STRING; }

   StringResource(std::string value) : value_(value) { }

   std::string GetValue() const { return value_; }

private:
   std::string    value_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_STRING_RESOURCE_H
