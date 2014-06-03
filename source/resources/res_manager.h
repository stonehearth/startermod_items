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
#include "lib/voxel/forward_defines.h"

BEGIN_RADIANT_RES_NAMESPACE

class ResourceManager2
{
public:
   ~ResourceManager2();

   static ResourceManager2& GetInstance();

   std::vector<std::string> const& GetModuleNames() const;

   void LookupManifest(std::string const& modname, std::function<void(Manifest const& m)> callback) const;
   void LookupJson(std::string const& path, std::function<void(JSONNode const& n)> callback) const;
   const JSONNode GetModules() const;


   AnimationPtr LookupAnimation(std::string const& path) const;

   std::string ConvertToCanonicalPath(std::string const& path, const char* search_ext) const;
   std::string FindScript(std::string const& script) const;

   std::string GetAliasUri(std::string const& mod_name, std::string const& alias_name) const;
   std::shared_ptr<std::istream> OpenResource(std::string const& stream) const;
   voxel::QubicleFile const* OpenQubicleFile(std::string const& name);

private:
   enum MixinMode {
      MIX_UNDER,
      MIX_OVER,
   };

   typedef std::unordered_map<std::string, voxel::QubicleFilePtr> QubicleMap;

private:
   JSONNode const& LookupJsonInternal(std::string const& path) const;
   Manifest LookupManifestInternal(std::string const& modname) const;
   ResourceManager2();
   static std::unique_ptr<ResourceManager2> singleton_;

   void ParseFilepath(std::string const& path,
                      std::string& canonical_path,
                      boost::filesystem::path& file_path,
                      const char* search_ext) const;
   AnimationPtr LoadAnimation(std::string const& canonical_path) const;
   JSONNode LoadJson(std::string const& path) const;
   void ParseNodeMixin(std::string const& path, JSONNode& node) const;
   void ApplyMixin(std::string const& mixin, JSONNode& n, MixinMode mode) const;
   void ExtendNode(JSONNode& node, const JSONNode& parent, MixinMode mode) const;
   std::string ExpandMacro(std::string const& current, std::string const& base_path, bool full) const;
   void ExpandMacros(std::string const& base_path, JSONNode& node, bool full) const;
   std::string ConvertToAbsolutePath(std::string const& current, std::string const& base_path) const;
   void LoadModules();
   void ImportModuleManifests();
   void ImportModMixintos(std::string const& modname, json::Node const& mixintos);
   void ImportModMixintoEntry(std::string const& modname, std::string const& name, std::string const& path);
   void ImportModOverrides(std::string const& modname, json::Node const& overrides);
   std::shared_ptr<std::istream> OpenResourceCanonical(std::string const& stream) const;

private:
   boost::filesystem::path                         resource_dir_;
   std::map<std::string, std::unique_ptr<IModule>> modules_;
   std::vector<std::string>                        module_names_;
   std::unordered_map<std::string, std::vector<std::string>> mixintos_;
   std::unordered_map<std::string, std::string>    overrides_;
   QubicleMap                                      qubicle_files_;
   mutable std::recursive_mutex                    mutex_;
   mutable std::unordered_map<std::string, AnimationPtr> animations_;
   mutable std::unordered_map<std::string, std::shared_ptr<JSONNode>>     jsons_;
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_RESOURCE_MANAGER_H
