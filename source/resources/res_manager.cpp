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
#include "radiant_json.h"
#include "res_manager.h"
#include "animation.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#include "sha.h"
#include "hex.h"
#include "files.h"

namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::res;

static const std::regex file_macro_regex__("^file\\((.*)\\)$");
static const std::regex entity_macro_regex__("^([^\\.\\\\/]+)\\.([^\\\\/]+)$");

// === Helper Functions ======================================================

static std::vector<std::string> SplitPath(std::string const& path)
{
   std::vector<std::string> s;
   boost::split(s, path, boost::is_any_of("/"));
   s.erase(std::remove(s.begin(), s.end(), ""), s.end());
   
   return s;
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
   std::ifstream json_in;
   OpenResource(canonical_path, json_in);

   std::string jsonfile = io::read_contents(json_in);
   std::string jsonhash = Checksum(jsonfile);
   fs::path binfile = fs::path(resource_dir_) / (jsonhash + std::string(".bin"));

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
      std::string type = json::ConstJsonObject(node).get<std::string>("type");
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

ResourceManager2::ResourceManager2() :
   resource_dir_("data")
{
   ASSERT(!singleton_);
   fs::directory_iterator end;
   for (fs::directory_iterator i(resource_dir_); i != end; i++) {
      fs::path path = i->path();
      if (fs::is_directory(path)) {
         moduleNames_.push_back(path.filename().string());
      }
   }
}

ResourceManager2::~ResourceManager2()
{
}

Manifest ResourceManager2::LookupManifest(std::string const& modname) const
{
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

void ResourceManager2::OpenResource(std::string const& canonical_path, std::ifstream& in) const
{
   std::string filepath = GetFilepath(canonical_path);

   in = std::ifstream(filepath, std::ios::in | std::ios::binary);
   if (!in.good()) {
      // how can we get here?  GetFilepath validates the filepath.
      // maybe if we can't open the file for some reason?
      throw InvalidFilePath(filepath);
   }
}

std::string ResourceManager2::GetResourceFileName(std::string const& path, const char* search_ext) const
{
   return GetFilepath(ConvertToCanonicalPath(path, search_ext));
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

std::string ResourceManager2::ConvertToCanonicalPath(std::string path, const char* search_ext) const
{
   path = ExpandMacro(path, ".", true); // so we can lookup things like 'stonehearth.wooden_axe'

   std::vector<std::string> parts = SplitPath(path);

   if (parts.size() < 1) {
      throw InvalidFilePath(path);
   }

   fs::path filepath = fs::path(resource_dir_);
   for (std::string part : parts) {
      filepath /= part;
   }

   // if the file path is a directory, look for a file with the name of
   // the last part in there (e.g. /foo/bar -> /foo/bar/bar.json
   if (fs::is_directory(filepath)) {
      std::string filename = filepath.filename().string() + search_ext;
      parts.push_back(filename);
      filepath /= filename;
   }

   if (!fs::is_regular_file(filepath)) {
      throw InvalidFilePath(path);
   }
   return boost::algorithm::join(parts, "/");
}


std::string ResourceManager2::GetFilepath(std::string const& canonical_path) const
{
   std::vector<std::string> parts = SplitPath(canonical_path);

   fs::path filepath = fs::path(resource_dir_);
   for (std::string const& part : parts) {
      filepath /= part;
   }

   std::wstring native_path = filepath.native();
   return std::string(native_path.begin(), native_path.end());
}

JSONNode ResourceManager2::LoadJson(std::string const& canonical_path) const
{
   std::string filepath = GetFilepath(canonical_path);

   std::ifstream in(filepath, std::ios::in);
   if (!in.good()) {
      throw InvalidFilePath(canonical_path);
   }

   std::stringstream reader;
   reader << in.rdbuf();

   json_string json = reader.str();
   if (!libjson::is_valid(json)) {
      throw InvalidJsonAtPathException(canonical_path);
   }

   JSONNode node = libjson::parse(json);
   ExpandMacros(canonical_path, node, false);
   ParseNodeExtension(canonical_path, node);

   return node;
}


void ResourceManager2::ParseNodeExtension(std::string const& path, JSONNode& node) const
{
   std::string extends = json::ConstJsonObject(node).get<std::string>("extends");
   if (!extends.empty()) {
      JSONNode parent;
      std::string uri = ExpandMacro(extends, extends, true);

      try {
         parent = LookupJson(uri);
      } catch (InvalidFilePath &) {
         throw InvalidFilePath(extends);
      }

      //LOG(WARNING) << "node pre-extend: " << node.write_formatted();
      //LOG(WARNING) << "extending with: " << parent.write_formatted();
      ExtendNode(node, parent);
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
   return moduleNames_;
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
   json::ConstJsonObject manifest = res::ResourceManager2::GetInstance().LookupManifest(mod_name);
   json::ConstJsonObject entities = manifest.getn("radiant").getn("entities");
   std::string uri = entities.get<std::string>(entity_name);
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
