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
#include "lib/json/node.h"
#include "res_manager.h"
#include "animation.h"
#include "core/config.h"
#include "directory_module.h"
#include "zip_module.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#include "sha.h"
#include "hex.h"
#include "files.h"

namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::res;

static const std::regex file_macro_regex__("^file\\((.*)\\)$");
static const std::regex entity_macro_regex__("^([^:\\\\/]+):([^\\\\/]+)$");

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
   fs::path animation_cache = core::Config::GetInstance().GetCacheDirectory() / "animations";
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

static std::vector<fs::path> GetPaths(fs::path const& directory, std::function<bool(fs::path const&)> filter_function)
{
   fs::directory_iterator const end;
   std::vector<fs::path> paths;

   for (fs::directory_iterator i(directory); i != end; i++) {
      fs::path const path = i->path();
      if (filter_function(path)) {
         paths.push_back(path);
      }
   }
   return paths;
}

ResourceManager2::ResourceManager2()
{
   ASSERT(!singleton_);

   resource_dir_ = "mods";

   // get a list of zip modules
   std::vector<fs::path> zip_paths;
   zip_paths = GetPaths(resource_dir_, [](fs::path const& path) {
      if (fs::is_regular_file(path) &&
          path.filename().extension() == ".smod") {
         return true;
      }
      return false;
   });

   // get a list of directory modules
   std::vector<fs::path> directory_paths;
   directory_paths = GetPaths(resource_dir_, [](fs::path const& path) {
      return fs::is_directory(path);
   });

   // zip modules have priority - install them first
   for (int i=0; i < zip_paths.size(); i++) {
      fs::path const& path = zip_paths[i];
      std::string const module_name = path.filename().stem().string();
      modules_[module_name] = std::unique_ptr<IModule>(new ZipModule(module_name, path));
      module_names_.push_back(module_name);
   }

   // install directory module only if zip module doesn't exist
   for (int i=0; i < directory_paths.size(); i++) {
      fs::path const& path = directory_paths[i];
      // don't stem - allow directories with '.'
      std::string const module_name = path.filename().string();

      if (modules_.find(module_name) == modules_.end()) {
         modules_[module_name] = std::unique_ptr<IModule>(new DirectoryModule(path));
         module_names_.push_back(module_name);
      }
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
      LOG(WARNING) << "error looking for manifest in " << modname << ": " << e.what();
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
   return modname + "/" + boost::algorithm::join(parts, "/");
}

JSONNode ResourceManager2::LoadJson(std::string const& canonical_path) const
{
   std::shared_ptr<std::istream> is = OpenResource(canonical_path);

   std::stringstream reader;
   reader << is->rdbuf();

   json_string json = reader.str();
   if (!libjson::is_valid(json)) {
      throw InvalidJsonAtPathException(canonical_path);
   }

   JSONNode node = libjson::parse(json);
   ExpandMacros(canonical_path, node, false);
   ParseNodeExtension(canonical_path, node);

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

void ResourceManager2::ParseNodeExtension(std::string const& path, JSONNode& n) const
{
   json::Node node(n);
   if (node.has("extends")) {
      std::string extends = node.get<std::string>("extends");
      JSONNode parent;
      std::string uri = ExpandMacro(extends, extends, true);

      try {
         parent = LookupJson(uri);
      } catch (InvalidFilePath &) {
         throw InvalidFilePath(extends);
      }

      //LOG(WARNING) << "node pre-extend: " << node.write_formatted();
      //LOG(WARNING) << "extending with: " << parent.write_formatted();
      ExtendNode(n, parent);
      //LOG(WARNING) << "node post-extend: " << node.write_formatted();
   }
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

std::string ResourceManager2::GetEntityUri(std::string const& mod_name, std::string const& entity_name) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   json::Node manifest = res::ResourceManager2::GetInstance().LookupManifest(mod_name);
   json::Node entities = manifest.get_node("radiant.entities");
   std::string uri = entities.get<std::string>(entity_name, "");

   if (uri.empty()) {
      std::ostringstream error;
      error << "'" << mod_name << "' has no entity named '" << entity_name << "' in the manifest.";
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
      if (std::regex_match(current, match, entity_macro_regex__)) {
         return GetEntityUri(match[1], match[2]);
      }
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
