#ifndef _RADIANT_RES_RESOURCE_MANAGER2_H
#define _RADIANT_RES_RESOURCE_MANAGER2_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include "libjson.h"
#include "animation.h"
#include "exceptions.h"
#include "manifest.h"

BEGIN_RADIANT_RES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   std::vector<std::string> const& GetModuleNames() const;
   Manifest LookupManifest(std::string const& modname) const;
   JSONNode const& LookupJson(std::string path) const;
   AnimationPtr LookupAnimation(std::string path) const;

   std::string ConvertToCanonicalPath(std::string path, const char* search_ext) const;
   std::string FindScript(std::string const& script) const;

   std::string GetAliasUri(std::string const& mod_name, std::string const& entity_name) const;
   std::shared_ptr<std::istream> OpenResource(std::string const& stream) const;

private:
   ResourceManager2();
   static std::unique_ptr<ResourceManager2> singleton_;

   void ParseFilepath(std::string const& path,
                      std::string& canonical_path,
                      boost::filesystem::path& file_path,
                      const char* search_ext) const;
   AnimationPtr LoadAnimation(std::string const& canonical_path) const;
   JSONNode LoadJson(std::string const& path) const;
   void ParseNodeExtension(std::string const& path, JSONNode& node) const;
   void ExtendNode(JSONNode& node, const JSONNode& parent) const;
   std::string ExpandMacro(std::string const& current, std::string const& base_path, bool full) const;
   void ExpandMacros(std::string const& base_path, JSONNode& node, bool full) const;
   std::string ConvertToAbsolutePath(std::string const& current, std::string const& base_path) const;

private:
   boost::filesystem::path                       resource_dir_;
   std::unordered_map<std::string, std::unique_ptr<IModule>>     modules_;
   std::vector<std::string>                      module_names_;
   mutable std::recursive_mutex                  mutex_;
   mutable std::unordered_map<std::string, AnimationPtr> animations_;
   mutable std::unordered_map<std::string, JSONNode>     jsons_;
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_RESOURCE_MANAGER_H
