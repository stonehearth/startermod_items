#ifndef _RADIANT_RESOURCES_RESOURCE_MANAGER2_H
#define _RADIANT_RESOURCES_RESOURCE_MANAGER2_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include "object_resource.h"
#include "libjson.h"
#include "data_resource.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   void LoadDirectory(std::string filename);
   void LoadJsonFile(std::string filename);

   const std::shared_ptr<Resource> Lookup(std::string id) const;

   template <class T> std::shared_ptr<T> Lookup(std::string id) const {
      std::shared_ptr<Resource> res = Lookup(id);
      if (res && res->GetType() == T::Type) {
         return std::static_pointer_cast<T>(res);
      }
      return nullptr;
   }

   template <> std::shared_ptr<DataResource> Lookup(std::string id) const {
      return std::dynamic_pointer_cast<DataResource>(Lookup(id));
   }

private:
   ResourceManager2();

private:
   void AddResourceToIndex(std::string key, std::shared_ptr<Resource> value);
   std::shared_ptr<Resource> ParseJson(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseJsonArray(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseJsonObject(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseGenericJsonObject(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseRigJsonObject(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseActionJsonObject(std::string ns, const JSONNode& json);
   std::shared_ptr<Resource> ParseAnimationJsonObject(std::string key, const JSONNode& json);
   std::shared_ptr<Resource> ParseRegion2dJsonObject(std::string key, const JSONNode& json);
   std::shared_ptr<Resource> ParseSkeletonJsonObject(std::string key, const JSONNode& json);
   std::shared_ptr<Resource> ParseDataJsonObject(Resource::ResourceType type, std::string key, const JSONNode& json);
   std::string ValidateJsonObject(const JSONNode& json);
   void ConvertJsonToBinFile(std::string jsonhash, std::string jsonfile, std::string binfile);
   std::string Checksum(std::string buffer);

private:
   static std::unique_ptr<ResourceManager2> singleton_;

private:
   mutable std::mutex                  mutex_;
   std::string                         resource_dir_;
   ObjectResource                      root_;
   std::unordered_map<std::string, std::shared_ptr<Resource>>   resources_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_MANAGER_H
