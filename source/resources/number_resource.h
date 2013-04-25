#ifndef _RADIANT_RESOURCES_NUMBER_RESOURCE_H
#define _RADIANT_RESOURCES_NUMBER_RESOURCE_H

#include "resource.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class NumberResource : public Resource {
public: 
   static ResourceType Type;
   ResourceType GetType() const override { return Resource::NUMBER; }

   NumberResource(double value) : value_(value) { }

   double GetValue() const { return value_; }

private:
   double         value_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_NUMBER_RESOURCE_H
