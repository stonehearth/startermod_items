#ifndef _RADIANT_RESOURCES_OBJECT_RESOURCE_H
#define _RADIANT_RESOURCES_OBJECT_RESOURCE_H

#include "resource.h"
#include "string_resource.h"
#include "number_resource.h"
#include <unordered_map>

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ObjectResource : public Resource {
public:
   static ResourceType Type;
   ResourceType GetType() const override { return Resource::OBJECT; }

   const std::shared_ptr<Resource> operator[](std::string key) const { 
      auto i = entries_.find(key);
      return i == entries_.end() ? nullptr : i->second;
   }

   template <class T> std::shared_ptr<T> Get(std::string key) const {
      std::shared_ptr<Resource> res = Get(key);
      if (res && res->GetType() == T::Type) {
         return std::static_pointer_cast<T>(res);
      }
      return nullptr;
   }

   std::string GetString(std::string key, std::string def = std::string()) const {
      auto res = Get<StringResource>(key);
      return res ? res->GetValue() : def;
   }
   int GetInteger(std::string key, int def = 0) const {
      auto res = Get<NumberResource>(key);
      return res ? (int)res->GetValue() : def;
   }
   float GetFloat(std::string key, float def = 0) const {
      auto res = Get<NumberResource>(key);
      return res ? (float)res->GetValue() : def;
   }

   std::shared_ptr<Resource> Get(std::string key) const { auto i = entries_.find(key); return i == entries_.end() ? nullptr : i->second; }
   std::shared_ptr<Resource>& operator[](std::string key) { return entries_[key]; }
   int GetSize() const { return entries_.size(); }

   typedef std::unordered_map<std::string, std::shared_ptr<Resource>> ResourceMap;

   const ResourceMap& GetContents() const { return entries_; }

private:
   ResourceMap    entries_;
};

typedef std::shared_ptr<ObjectResource> ObjectResourcePtr;

std::ostream& operator<<(std::ostream& out, const ObjectResource& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_OBJECT_RESOURCE_H
