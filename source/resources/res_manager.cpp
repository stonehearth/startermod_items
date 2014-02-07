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

static std::string Checksum(std::string input)
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
      resource_path = ConvertToCanonicalPath(path, nullptr);
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
         return;
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
         return;
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
      json::Node manifest = LookupManifest(modname);
      ImportModOverrides(modname, manifest.get_node("overrides"));
      ImportModMixintos(modname, manifest.get_node("mixintos"));
   }
}

ResourceManager2::~ResourceManager2()
{
}

Manifest ResourceManager2::LookupManifest(std::string const& modname) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   JSONNode result;
   try {
      result = LookupJson(modname + "/manifest.json");
   } catch (Exception &e) {
      RES_LOG(1) << "error looking for manifest in " << modname << ": " << e.what();
   }
   return Manifest(modname, result);
}

JSONNode const& ResourceManager2::LookupJson(std::string path) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   
   std::string key = ConvertToCanonicalPath(path, ".json");

   auto i = jsons_.find(key);
   if (i != jsons_.end()) {
      return i->second;
   }
   return (jsons_[key] = LoadJson(key));
}

AnimationPtr ResourceManager2::LookupAnimation(std::string path) const
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
   
   std::string modname;
   std::vector<std::string> parts;

   std::string canonical_path = ConvertToCanonicalPath(path, nullptr);
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

std::string ResourceManager2::ConvertToCanonicalPath(std::string path, const char* search_ext) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   path = ExpandMacro(path, ".", true); // so we can lookup things like 'stonehearth:wooden_axe'

   std::string modname;
   std::vector<std::string> parts;
   ParsePath(path, modname, parts);

   auto i = modules_.find(modname);
   if (i == modules_.end()) {
      throw InvalidFilePath(path);
   }

   if (!i->second->CheckFilePath(parts)) {
      if (!search_ext) {
         throw InvalidFilePath(path);
      }
      // try the 2nd form...
      parts.push_back(parts.back() + search_ext);
      if (!i->second->CheckFilePath(parts)) {
         throw InvalidFilePath(path);
      }
   }

   std::string canonical_path = modname + "/" + boost::algorithm::join(parts, "/");

   // finally, see if this resource has been overridden.  If so, use that path instead
   auto j = overrides_.find(canonical_path);
   if (j != overrides_.end()) {
      RES_LOG(3) << "overriding path " << path << " with " << j->second;
      canonical_path = j->second;
   }
   return canonical_path;
}

JSONNode ResourceManager2::LoadJson(std::string const& canonical_path) const
{
   JSONNode node;
   std::string error_message;
   bool success;

   std::shared_ptr<std::istream> is = OpenResource(canonical_path);
   success = json::ReadJson(*is, node, error_message);
   if (!success) {
      std::string full_message = BUILD_STRING("Error reading file " << canonical_path << ":\n\n" << error_message);
      RES_LOG(1) << full_message;
      throw core::Exception(full_message);
   }

   ExpandMacros(canonical_path, node, false);
   ParseNodeMixin(canonical_path, node);

   return node;
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

void ResourceManager2::ParseNodeMixin(std::string const& path, JSONNode& n) const
{
   json::Node node(n);
   // Parse the node mixins first
   if (node.has("mixins")) {
      json::Node mixins = node.get_node("mixins");
      if (mixins.type() == JSON_STRING) {
         std::string mixin = node.get<std::string>("mixins");
         // if the node is an array, load this shit in in a loop
         ApplyMixin(mixin, n, node);
      } else if (mixins.type() == JSON_ARRAY) {
         for (JSONNode const &child : mixins.get_internal_node()) {
            std::string mixin = child.as_string();
            ApplyMixin(mixin, n, node);
         }
      } else {
         throw json::InvalidJson("invalid json node type in mixin. Mixins must be a string or an array.");
      }
   }

   // Now apply the mixintos from other modules
   auto i = mixintos_.find(path);
   if (i != mixintos_.end()) {
      for (std::string const& mixin : i->second) {
         ApplyMixin(mixin, n, node);
      }
   }
}

void ResourceManager2::ApplyMixin(std::string const& mixin, JSONNode& n, json::Node node) const
{
   JSONNode parent;
   std::string uri = ExpandMacro(mixin, mixin, true);

   try {
      parent = LookupJson(uri);
   } catch (InvalidFilePath &) {
      throw InvalidFilePath(mixin);
   }

   RES_LOG(7) << "node pre-mixin: " << node.write_formatted();
   RES_LOG(7) << "mixing with: " << parent.write_formatted();
   ExtendNode(n, parent);
   RES_LOG(7) << "node post-mixin: " << node.write_formatted();

}

void ResourceManager2::ExtendNode(JSONNode& node, const JSONNode& parent) const
{
   ASSERT(node.type() == parent.type());

   switch (parent.type()) {
   case JSON_NODE:
      {
         for (const auto& i : parent) {
            std::string const& name = i.name();
            auto current = node.find(name);
            if (current!= node.end()) {
               ExtendNode(*current, i);
            } else {
               node.push_back(i);
            }
         }
         break;
      }
   case JSON_ARRAY:
      {
         JSONNode values = parent;
         for (uint i = 0; i < node.size(); i++) {
            values.push_back(node[i]);
         }
         node = values;
         break;
      }
   default:
      node = parent;
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

   json::Node manifest = LookupManifest(mod_name);
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
