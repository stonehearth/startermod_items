#ifndef _RADIANT_RESOURCES_RESOURCE_MANAGER2_H
#define _RADIANT_RESOURCES_RESOURCE_MANAGER2_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include "libjson.h"
#include "animation.h"
#include "exceptions.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   JSONNode const& LookupManifest(std::string const& modname) const;
   JSONNode const& LookupJson(std::string path) const;
   AnimationPtr LookupAnimation(std::string path) const;

   void OpenResource(std::string const& path, std::ifstream& in) const;
   std::string GetResourceFileName(std::string const& path, const char* serach_ext) const;  // xxx: used only for lua... it's bad!
   std::string ConvertToCanonicalPath(std::string const& path, const char* search_ext) const;

private:
   ResourceManager2();
   static std::unique_ptr<ResourceManager2> singleton_;

   void ParseFilepath(std::string const& path,
                      std::string& canonical_path,
                      boost::filesystem::path& file_path,
                      const char* search_ext) const;
   std::string GetFilepath(std::string const& path) const;
   AnimationPtr LoadAnimation(std::string const& canonical_path) const;
   JSONNode LoadJson(std::string const& path) const;
   void ParseNodeExtension(std::string const& path, JSONNode& node) const;
   void ExtendNode(JSONNode& node, const JSONNode& parent) const;
   void ConvertToAbsolutePaths(std::string const& base_path, JSONNode& node) const;

private:
   boost::filesystem::path                       resource_dir_;
   mutable std::recursive_mutex                  mutex_;
   mutable std::unordered_map<std::string, AnimationPtr> animations_;
   mutable std::unordered_map<std::string, JSONNode>     jsons_;
};

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_RESOURCE_MANAGER_H
