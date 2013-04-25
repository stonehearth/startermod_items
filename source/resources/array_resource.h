#ifndef _RADIANT_RESOURCES_ARRAY_RESOURCE_H
#define _RADIANT_RESOURCES_ARRAY_RESOURCE_H

#include "resource.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ArrayResource : public Resource {
public: 
   static ResourceType Type;
   ResourceType GetType() const override { return Resource::ARRAY; }

   const std::shared_ptr<Resource> Get(int i) const { return entries_[i]; }
   const std::shared_ptr<Resource> operator[](int i) const { return entries_[i]; }
   std::shared_ptr<Resource>& operator[](int i) { return entries_[i]; }

   void Append(std::shared_ptr<Resource> value) { entries_.push_back(value); }
   int GetSize() const { return entries_.size(); }

   typedef std::vector<std::shared_ptr<Resource>> ResourceVector;

   const ResourceVector& GetContents() const { return entries_; }

private:

   ResourceVector    entries_;
};

std::ostream& operator<<(std::ostream& out, const ArrayResource& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_ARRAY_RESOURCE_H
