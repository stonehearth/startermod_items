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
using namespace ::radiant::resources;


// === Helper Functions ======================================================

static std::vector<std::string> SplitPath(std::string const& path)
{
   std::vector<std::string> s;
   boost::split(s, path, boost::is_any_of("/"));
   s.erase(std::remove(s.begin(), s.end(), ""), s.end());
   
   return s;
}

static std::string ConvertToAbsolutePath(std::string const& current, std::string const& p)
{
   if (!boost::starts_with(p, "file(") || !boost::ends_with(p, ")")) {
      return p;
   }

   std::string path = p.substr(5, p.size() - 6);
   std::vector<std::string> newpath;

   if (path[0] == '/') {
      // absolute paths (relative the the current mod).
      std::vector<std::string> current_path = SplitPath(current);
      newpath.push_back(*current_path.begin());
   } else {
      // relatives paths (to the current directory)
      newpath = SplitPath(current);
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
      std::string type = json::get<std::string>(node, "type", "");
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
}

ResourceManager2::~ResourceManager2()
{
}

JSONNode const& ResourceManager2::LookupManifest(std::string const& modname) const
{
   return LookupJson(modname + "/manifest.json");
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

void ResourceManager2::ConvertToAbsolutePaths(std::string const& base_path, JSONNode& node) const
{
   int type = node.type();
   if (type == JSON_NODE || type == JSON_ARRAY) {
      for (auto& i : node) {
         ConvertToAbsolutePaths(base_path, i);
      }
   } else if (type == JSON_STRING) {
      std::string absolute_path = ConvertToAbsolutePath(base_path, node.as_string());
      node = JSONNode(node.name(), absolute_path);
   }
}

std::string ResourceManager2::ConvertToCanonicalPath(std::string const& path, const char* search_ext) const
{
   std::vector<std::string> parts = SplitPath(path);

   if (parts.size() < 1) {
      throw InvalidUriException(path);
   }

   fs::path filepath = fs::path(resource_dir_);
   for (std::string const& part : parts) {
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
      throw InvalidUriException(path);
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
      throw InvalidUriException(canonical_path);
   }

   std::stringstream reader;
   reader << in.rdbuf();

   json_string json = reader.str();
   if (!libjson::is_valid(json)) {
      throw InvalidJsonAtUriException(canonical_path);
   }

   JSONNode node = libjson::parse(json);
   ConvertToAbsolutePaths(canonical_path, node);
   ParseNodeExtension(canonical_path, node);

   return node;
}


void ResourceManager2::ParseNodeExtension(std::string const& path, JSONNode& node) const
{
   std::string extends = json::get<std::string>(node, "extends", "");
   if (!extends.empty()) {
      std::string absolute_path = ConvertToAbsolutePath(path, extends);

      JSONNode const& parent = LookupJson(absolute_path);

      LOG(WARNING) << "node pre-extend: " << node.write_formatted();
      LOG(WARNING) << "extending with: " << parent.write_formatted();
      ExtendNode(node, parent);
      LOG(WARNING) << "node post-extend: " << node.write_formatted();
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