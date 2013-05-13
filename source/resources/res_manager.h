#ifndef _RADIANT_RESOURCES_RESOURCE_MANAGER2_H
#define _RADIANT_RESOURCES_RESOURCE_MANAGER2_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include "libjson.h"
#include "data_resource.h"

namespace boost { namespace network { namespace uri { class uri; } } }

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   const std::shared_ptr<Resource> LookupResource(std::string id);
   bool OpenResource(std::string const& uri, std::ifstream& in);
   bool OpenResource(boost::network::uri::uri const& uri, std::ifstream& in);

   template <class T> std::shared_ptr<T> Lookup(std::string id) {
      std::shared_ptr<Resource> res = LookupResource(id);
      if (res && res->GetType() == T::Type) {
         return std::static_pointer_cast<T>(res);
      }
      return nullptr;
   }
   template <> std::shared_ptr<DataResource> Lookup(std::string id) {
      return std::dynamic_pointer_cast<DataResource>(LookupResource(id));
   }

private:
   ResourceManager2();

private:
   bool ParseUri(std::string const& uristr, boost::network::uri::uri &uri);
   std::shared_ptr<Resource> ParseAnimationJsonObject(std::string key, const JSONNode& json);
   std::shared_ptr<Resource> ParseDataJsonObject(Resource::ResourceType type, std::string key, const JSONNode& json);
   std::string Checksum(std::string buffer);
   void ExtendRootJsonNode(boost::network::uri::uri const& uri, JSONNode& node);
   void ExtendNode(JSONNode& node, const JSONNode& parent);
   std::string ExpandURL(boost::network::uri::uri const& current, std::string const& str);
   JSONNode ExpandJSON(boost::network::uri::uri const& uri, JSONNode const& node);
   bool LoadJson(boost::network::uri::uri const& uri, JSONNode& node);

private:
   static std::unique_ptr<ResourceManager2> singleton_;

private:
   typedef std::unordered_map<std::string, JSONNode> ExtendedNodesMap;
   mutable std::mutex                  mutex_;
   std::string                         resource_dir_;
   ExtendedNodesMap                    extendedNodes_;
   std::unordered_map<std::string, std::shared_ptr<Resource>>   resources_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_MANAGER_H
