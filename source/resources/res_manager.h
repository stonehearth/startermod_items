#ifndef _RADIANT_RESOURCES_RESOURCE_MANAGER2_H
#define _RADIANT_RESOURCES_RESOURCE_MANAGER2_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include "libjson.h"
#include "animation.h"
#include "exceptions.h"

namespace boost { namespace network { namespace uri { class uri; } } }

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   JSONNode const& LookupJson(std::string uri) const;
   AnimationPtr LookupAnimation(std::string uri) const;

   void OpenResource(std::string const& uri, std::ifstream& in) const;
   std::string GetResourceFileName(std::string const& uri, const char* serach_ext) const;  // xxx: used only for lua... it's bad!
   boost::network::uri::uri ConvertToCanonicalUri(boost::network::uri::uri const& uri, const char* search_ext) const;

private:
   ResourceManager2();
   static std::unique_ptr<ResourceManager2> singleton_;

   void ParseUriAndFilepath(boost::network::uri::uri const& uri,
                            boost::network::uri::uri &canonical_uri,
                            std::string& path,
                            const char* search_ext) const;
   std::string GetFilepathForUri(boost::network::uri::uri const& uri) const;
   AnimationPtr LoadAnimation(boost::network::uri::uri const& canonical_uri) const;
   JSONNode LoadJson(boost::network::uri::uri const& uri) const;
   void ParseNodeExtension(boost::network::uri::uri const& uri, JSONNode& node) const;
   void ExtendNode(JSONNode& node, const JSONNode& parent) const;
   void ConvertToAbsoluteUris(boost::network::uri::uri const& canonical_uri, JSONNode& node) const;
   

private:
   boost::filesystem::path                       resource_dir_;
   mutable std::recursive_mutex                  mutex_;
   mutable std::unordered_map<std::string, AnimationPtr> animations_;
   mutable std::unordered_map<std::string, JSONNode>     jsons_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_MANAGER_H
