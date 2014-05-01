#include <fstream>
#include <sstream>
#include <io.h>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "radiant.h"
#include "radiant_file.h"
#include "radiant_stdutil.h"
#include "lib/json/node.h"
#include "lib/json/json.h"
#include "res_manager.h"
#include "animation.h"
#include "core/config.h"
#include "core/system.h"
#include "directory_module.h"
#include "zip_module.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#include "sha.h"
#include "hex.h"
#include "files.h"

namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::res;

#define RES_LOG(level)     LOG(resources, level)

static const std::regex file_macro_regex__("^file\\((.*)\\)$");
static const std::regex alias_macro_regex__("^([^:\\\\/]+):([^\\\\/]+)$");

// === Helper Functions ======================================================

static std::vector<std::string> SplitPath(std::string const& path)
{
   std::vector<std::string> s;
   boost::split(s, path, boost::is_any_of("/"));
   s.erase(std::remove(s.begin(), s.end(), ""), s.end());

   return s;
}

static void ParsePath(std::string const& path, std::string& modname, std::vector<std::string>& parts)
{
   std::vector<std::string> s = SplitPath(path);
   
   parts.clear();
   if (s.size() > 0) {
      modname = *s.begin();
      std::copy(s.begin() + 1, s.end(), std::back_inserter(parts));
   } else {
      modname = "";
   }
}

static std::string Checksum(std::string const& input)
{
   CryptoPP::SHA256 hash;
   byte buffer[2 * CryptoPP::SHA256::DIGESTSIZE]; // Output size of the buffer

   CryptoPP::StringSource(input, true,   // true here means consume all input at once 
      new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(new CryptoPP::ArraySink(buffer, sizeof buffer))));

   return std::string((const char *)buffer, sizeof buffer);
}

AnimationPtr ResourceManager2::LoadAnimation(std::string const& canonical_path) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   std::shared_ptr<std::istream> json_in = OpenResource(canonical_path);

   std::string jsonfile = io::read_contents(*json_in);
   std::string jsonhash = Checksum(jsonfile);
   fs::path animation_cache = core::System::GetInstance().GetTempDirectory() / "animations";
   if (!fs::is_directory(animation_cache)) {
      fs::create_directories(animation_cache);
   }
   fs::path binfile = animation_cache / (jsonhash + std::string(".bin"));

   std::string buffer;
   std::ifstream in(binfile.string(), std::ios::out | std::ios::binary);
   if (in.good()) {
      std::string binhash;
      std::getline(in, binhash, '\0');
      if (binhash == jsonhash) {
         std::stringstream reader;
         reader << in.rdbuf();
         buffer = reader.str();
      }
      in.close();
   }
   if (buffer.empty()) {
      JSONNode node = libjson::parse(jsonfile);
      std::string type = json::Node(node).get<std::string>("type", "");
      if (type.empty()) {
         throw InvalidResourceException(canonical_path, "'type' field missing");
      }
      if (type != "animation") {
         throw InvalidResourceException(canonical_path, "node type is not 'animation'");
      }      
      buffer = Animation::JsonToBinary(node);
      std::ofstream out(binfile.string(), std::ios::out | std::ios::binary);
      out << jsonhash << '\0';
      out.write(&buffer[0], buffer.size());
      out.close();
   }
   return std::make_shared<Animation>(buffer);
}

// === Methods ===============================================================

std::unique_ptr<ResourceManager2> ResourceManager2::singleton_;

ResourceManager2& ResourceManager2::GetInstance()
{
   if (!singleton_) {
      singleton_.reset(new ResourceManager2());
   }
   return *singleton_;
}

ResourceManager2::ResourceManager2()
{
   ASSERT(!singleton_);

   resource_dir_ = "mods";
   LoadModules();
   ImportModuleManifests();
}

void ResourceManager2::LoadModules()
{
   fs::directory_iterator const end;
   std::vector<fs::path> paths;
   
   for (fs::directory_iterator i(resource_dir_); i != end; i++) {
      fs::path const path = i->path();

      // zip modules have priority - install them first
      if (fs::is_regular_file(path) && path.filename().extension() == ".smod") {
         std::string const module_name = path.filename().stem().string();
         
         ASSERT(!stdutil::contains(modules_, module_name));
         modules_[module_name] = std::unique_ptr<IModule>(new ZipModule(module_name, path));
      } 

      // load directories only if the coresponding zip file does not exist
      if (fs::is_directory(path)) {
         std::string const module_name = path.filename().string();
         std::string const potential_smod = module_name + ".smod";
         if (!fs::is_regular_file(resource_dir_ / (module_name + ".smod"))) {
            ASSERT(!stdutil::contains(modules_, module_name));
            modules_[module_name] = std::unique_ptr<IModule>(new DirectoryModule(path));
         }
      }
   }
   // Compute the module names last so we ensure the list is in sorted order
   for (const auto& entry : modules_) {
      module_names_.push_back(entry.first);
   }
}

void ResourceManager2::ImportModMixintoEntry(std::string const& modname, std::string const& name, std::string const& path)
{
   std::string resource_path;
   try {
      resource_path = ConvertToCanonicalPath(path, ".json");
   } catch (std::exception const&) {
      RES_LOG(1) << "could not find resource for " << path << " while processing mixintos for " << modname << ".  ignoring.";
      return;
   }
   mixintos_[name].push_back(resource_path);
   RES_LOG(3) << "adding " << resource_path << " to mixins for " << name;
}

void ResourceManager2::ImportModMixintos(std::string const& modname, json::Node const& mixintos)
{
   for (json::Node const& mixinto : mixintos) {
      // by now all the modules have been loaded, so we have a complete alias map.
      // break the alias for the left-hand-side all the way down to a file before
      // storing.
      std::string mixinto_path;
      try {
         mixinto_path = ConvertToCanonicalPath(mixinto.name(), ".json");
      } catch (std::exception const&) {
         RES_LOG(1) << "could not find resource for " << mixinto.name() << " while processing mixintos for " << modname << ".  ignoring.";
         continue;
      }
      if (mixinto.type() == JSON_STRING) {
         ImportModMixintoEntry(modname, mixinto_path, mixinto.as<std::string>());
      } else if (mixinto.type() == JSON_ARRAY) {
         for (json::Node const& path : mixinto) {
            ImportModMixintoEntry(modname, mixinto_path, path.as<std::string>());
         }
      } else {
         RES_LOG(1) << "mixintos entry " << mixinto_path << " in mod " << modname << " is not a string or array.  ignoring";
      }
   }
}

void ResourceManager2::ImportModOverrides(std::string const& modname, json::Node const& overrides)
{
   for (json::Node const& over : overrides) {
      std::string override_path, resource_path;
      try {
         override_path = ConvertToCanonicalPath(over.name(), ".json");
      } catch (std::exception const&) {
         RES_LOG(1) << "could not find resource for " << over.name() << " while processing overrides for " << modname << ".  ignoring.";
         continue;
      }
      try {
         resource_path = ConvertToCanonicalPath(over.as<std::string>(), ".json");
      } catch (std::exception const&) {
         RES_LOG(1) << "could not find resource for " << over.as<std::string>() << " while processing mixintos for " << modname << ".  ignoring.";
         return;
      }
      overrides_[override_path] = resource_path;
      RES_LOG(3) << "adding " << resource_path << " to overrides for " << over.name();
   }
}

void ResourceManager2::ImportModuleManifests()
{
   // modules_ is a std::map<>, so this will process modules in alphabetical order.
   // While not ideal, at least it's deterministic!  We can try to do better in a
   // future version if modders need it
   for (std::string const& modname : module_names_) {
      json::Node manifest = LookupManifestInternal(modname);
      ImportModOverrides(modname, manifest.get_node("overrides"));
      ImportModMixintos(modname, manifest.get_node("mixintos"));
   }
}

ResourceManager2::~ResourceManager2()
{
}

Manifest ResourceManager2::LookupManifestInternal(std::string const& modname) const
{
   JSONNode result;
   try {
      result = LookupJsonInternal(modname + "/manifest.json");
   } catch (Exception &e) {
      RES_LOG(1) << "error looking for manifest in " << modname << ": " << e.what();
   }
   return Manifest(modname, result);
}

void ResourceManager2::LookupManifest(std::string const& modname, std::function<void(Manifest const& m)> callback) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   callback(LookupManifestInternal(modname));
}


void ResourceManager2::LookupJson(std::string const& path, std::function<void(JSONNode const& n)> callback) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   callback(LookupJsonInternal(path));
}

JSONNode const& ResourceManager2::LookupJsonInternal(std::string const& path) const
{
   std::shared_ptr<JSONNode> node;

   // If we have already loaded the json for this file, it should already be in the
   // map.  ConvertToCanonicalPath is quite expensive.  We need to iterate over
   // the mixins and overrides tables and everything!  To avoid that, we store the
   // json object both in the canonical location and whatever aliases people are
   // using to reach that path
   auto i = jsons_.find(path), end = jsons_.end();
   if (i != end) {
      node = i->second;
   } else {
      // Couldn't find it.  Rats.  See if we've already loaded it using a different
      // alias.  If so, both aliases will map to the same canonical path
      std::string canonical_path = ConvertToCanonicalPath(path, ".json");
   
      i = jsons_.find(canonical_path);
      if (i != end) {
         // Great!  It was loaded by a previous alias.  Wire up this alias, too, so we
         // can avoid calling ConvertToCanonicalPath next time someone looks it up.
         node = i->second;
         jsons_[path] = i->second;
      } else {
         // Still couldn't find it?  Load the node and store it under the canonical path
         // and the current key so we can find it next time (using either!)
         node = std::make_shared<JSONNode>(LoadJson(canonical_path));
         jsons_[path] = jsons_[canonical_path] = node;
      }
   }
   ASSERT(node);  // We should have a node or have thrown an exception if the path was invalid.
   return *node;
}

const JSONNode ResourceManager2::GetModules() const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   JSONNode result;
   for (std::string const& modname : GetModuleNames()) {
      JSONNode manifest;
      try {
         manifest = LookupManifestInternal(modname).get_internal_node();
      } catch (std::exception const&) {
         // Just use an empty manifest...f
      }
      manifest.set_name(modname);
      result.push_back(manifest);
   }

   return result;
}

AnimationPtr ResourceManager2::LookupAnimation(const std::string& path) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   std::string key = ConvertToCanonicalPath(path, ".json");   

   auto i = animations_.find(key);
   if (i != animations_.end()) {
      return i->second;
   }
   return (animations_[key] = LoadAnimation(key));
}

std::shared_ptr<std::istream> ResourceManager2::OpenResource(std::string const& path) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   
   std::string canonical_path = ConvertToCanonicalPath(path, nullptr);
   return OpenResourceCanonical(canonical_path);
}

std::shared_ptr<std::istream> ResourceManager2::OpenResourceCanonical(std::string const& canonical_path) const
{
   std::string modname;
   std::vector<std::string> parts;

   ParsePath(canonical_path, modname, parts);

   auto i = modules_.find(modname);
   if (i == modules_.end()) {
      throw InvalidFilePath(canonical_path);
   }

   std::string filename = boost::algorithm::join(parts, "/");
   std::shared_ptr<std::istream> result = i->second->OpenResource(filename);
   if (!result) {
      throw InvalidFilePath(canonical_path);
   }
   return result;

}

std::string ResourceManager2::ConvertToCanonicalPath(const std::string& path, const char* search_ext) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   std::string path_exp = ExpandMacro(path, ".", true); // so we can lookup things like 'stonehearth:wooden_axe'

   std::string modname;
   std::vector<std::string> parts;
   ParsePath(path_exp, modname, parts);

   auto i = modules_.find(modname);
   if (i == modules_.end()) {
      throw InvalidFilePath(path_exp);
   }

   if (!i->second->CheckFilePath(parts)) {
      if (!search_ext) {
         throw InvalidFilePath(path_exp);
      }
      // try the 2nd form...
      parts.push_back(parts.back() + search_ext);
      if (!i->second->CheckFilePath(parts)) {
         throw InvalidFilePath(path_exp);
      }
   }

   std::string canonical_path = modname + "/" + boost::algorithm::join(parts, "/");

   // finally, see if this resource has been overridden.  If so, use that path instead
   auto j = overrides_.find(canonical_path);
   if (j != overrides_.end()) {
      RES_LOG(3) << "overriding path " << path_exp << " with " << j->second;
      canonical_path = j->second;
   }
   return canonical_path;
}

JSONNode ResourceManager2::LoadJson(std::string const& canonical_path) const
{
   JSONNode new_node;
   std::string error_message;
   bool success;

   std::shared_ptr<std::istream> is = OpenResourceCanonical(canonical_path);
   success = json::ReadJson(*is, new_node, error_message);
   if (!success) {
      std::string full_message = BUILD_STRING("Error reading file " << canonical_path << ":\n\n" << error_message);
      RES_LOG(1) << full_message;
      throw core::Exception(full_message);
   }

   ExpandMacros(canonical_path, new_node, false);
   ParseNodeMixin(canonical_path, new_node);

   return new_node;
}

void ResourceManager2::ExpandMacros(std::string const& base_path, JSONNode& node, bool full) const
{
   int type = node.type();
   if (type == JSON_NODE || type == JSON_ARRAY) {
      for (auto& i : node) {
         ExpandMacros(base_path, i, full);
      }
   } else if (type == JSON_STRING) {
      std::string expanded = ExpandMacro(node.as_string(), base_path, full);
      node = JSONNode(node.name(), expanded);
   }
}

void ResourceManager2::ParseNodeMixin(std::string const& path, JSONNode& new_node) const
{
   RES_LOG(5) << "node " << path << " pre-all-mixins: " << new_node.write_formatted();

   json::Node node(new_node);
   // Parse the node mixins first
   if (node.has("mixins")) {
      json::Node mixins = node.get_node("mixins");
      if (mixins.type() == JSON_STRING) {
         std::string mixin = node.get<std::string>("mixins");
         ApplyMixin(mixin, new_node, MIX_UNDER);
      } else if (mixins.type() == JSON_ARRAY) {
         // if the node is an array, load this shit in in a loop
         for (JSONNode const &child : mixins.get_internal_node()) {
            std::string mixin = child.as_string();
            ApplyMixin(mixin, new_node, MIX_UNDER);
         }
      } else {
         throw json::InvalidJson("invalid json node type in mixin. Mixins must be a string or an array.");
      }
   }

   // Now apply the mixintos from other modules
   auto i = mixintos_.find(path);
   if (i != mixintos_.end()) {
      for (std::string const& mixin : i->second) {
         ApplyMixin(mixin, new_node, MIX_OVER);
      }
   }

   RES_LOG(5) << "node " << path << " post-all-mixins: " << new_node.write_formatted();
}

void ResourceManager2::ApplyMixin(std::string const& mixin_name, JSONNode& new_node, MixinMode mode) const
{
   JSONNode mixin_node;
   std::string uri = ExpandMacro(mixin_name, mixin_name, true);

   try {
      mixin_node = LookupJsonInternal(uri);
   } catch (InvalidFilePath &) {
      throw InvalidFilePath(mixin_name);
   }

   RES_LOG(7) << "node pre-mixin: " << new_node.write_formatted();
   RES_LOG(7) << "mixing with (mixin:" << mixin_name << " uri:" << uri << ") : " << mixin_node.write_formatted();
   ExtendNode(new_node, mixin_node, mode);
   RES_LOG(7) << "node post-mixin: " << new_node.write_formatted();
}

void ResourceManager2::ExtendNode(JSONNode& new_node, const JSONNode& mixin_node, MixinMode mode) const
{
   ASSERT(new_node.name() == "mixins" || new_node.type() == mixin_node.type());

   switch (mixin_node.type()) {
   case JSON_NODE:
      {
         for (const auto& i : mixin_node) {
            std::string const& name = i.name();
            auto current = new_node.find(name);
            if (current != new_node.end()) {
               RES_LOG(9) << "extending new_node key " << name << " with mixin_node";
               ExtendNode(*current, i, mode);
            } else {
               RES_LOG(9) << "adding new_node key " << name << " from mixin_node";
               new_node.push_back(i);
            }
         }
         break;
      }
   case JSON_ARRAY:
      {
         RES_LOG(9) << "adding " << mixin_node.size() << " array values to existing " << new_node.size() << " values";
         JSONNode values = mixin_node;
         for (uint i = 0; i < new_node.size(); i++) {
            values.push_back(new_node[i]);
         }
         new_node = values;
         break;
      }
   default:
      RES_LOG(9) << "replacing new_node with mixin_node";
      ASSERT(new_node.name() == mixin_node.name());
      if (mode == MIX_UNDER) {
         // we want to mix the mixin_node underneath the new_node.  So the new_node should
         // override anything that appears in the mixin with the current name.  Therefore,
         // the right course of action is to do nothing, since we know the names match.
      } else if (mode == MIX_OVER) {
         // we want to mix the mixin_node on *top* of the new_node.  So clobber it.
         new_node = mixin_node;
      } else {
         // Logically unreachable.
         ASSERT(false);
      }
      break;      
   }
}

std::vector<std::string> const& ResourceManager2::GetModuleNames() const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   return module_names_;
}

std::string ResourceManager2::ConvertToAbsolutePath(std::string const& path, std::string const& base_path) const
{
   std::vector<std::string> newpath;

   if (path[0] == '/') {
      // absolute paths (relative the the current mod).
      std::vector<std::string> current_path = SplitPath(base_path);
      newpath.push_back(*current_path.begin());
   } else {
      // relatives paths (to the base_path directory)
      newpath = SplitPath(base_path);
      newpath.pop_back();
   }

   for (auto const& elem : SplitPath(path)) {
      if (elem == ".") {
      } else if (elem == "..") {
         newpath.pop_back();
      } else {
         newpath.push_back(elem);
      }
   }
   return std::string("/") + boost::algorithm::join(newpath, "/");
}

std::string ResourceManager2::GetAliasUri(std::string const& mod_name, std::string const& alias_name) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   json::Node manifest = LookupManifestInternal(mod_name);
   json::Node aliases = manifest.get_node("aliases");
   std::string uri = aliases.get<std::string>(alias_name, "");

   if (uri.empty()) {
      std::ostringstream error;
      error << "'" << mod_name << "' has no alias named '" << alias_name << "' in the manifest.";
      throw res::Exception(error.str());
   }
   return uri;
}

std::string ResourceManager2::ExpandMacro(std::string const& current, std::string const& base_path, bool full) const
{
   std::smatch match;

   if (std::regex_match(current, match, file_macro_regex__)) {
      return ConvertToAbsolutePath(match[1], base_path);
   }
   if (full) {
      std::string translated = current;
      if (std::regex_match(current, match, alias_macro_regex__)) {
         translated = GetAliasUri(match[1], match[2]);
      }
      return translated;
   }
   return current;
}

std::string ResourceManager2::FindScript(std::string const& script) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   std::string path = ExpandMacro(script, ".", true); // so we can lookup things like 'stonehearth:sleep_action'
   if (!boost::ends_with(path, ".lua")) {
      throw std::logic_error("expected script path to end with .lua");
   }

   std::string modname;
   std::vector<std::string> parts;
   ParsePath(path, modname, parts);

   auto i = modules_.find(modname);
   if (i == modules_.end()) {
      throw InvalidFilePath(path);
   }

   if (!i->second->CheckFilePath(parts)) {
      // try the compiled version...
      parts.back() += "c";
      if (!i->second->CheckFilePath(parts)) {
         throw InvalidFilePath(script);
      }
   }
   return modname + "/" + boost::algorithm::join(parts, "/");
}
