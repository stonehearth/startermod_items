#include <fstream>
#include <sstream>
#include <io.h>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
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

namespace network = ::boost::network;
namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::resources;

// === Helper Functions ======================================================

static network::uri::uri ConvertToAbsoluteUri(network::uri::uri const& current, std::string const& str)
{
   if (!boost::starts_with(str, "url(") || !boost::ends_with(str, ")")) {
      return network::uri::uri(str);
   }

   fs::path newpath;
   std::string suffix = str.substr(4, str.size() - 5);
   if (!suffix.empty() && suffix[0] == '/') {
      // start at the root
      newpath = fs::path("/");
      suffix = suffix.substr(1);
   } else {
      // start in the current directory
      newpath = fs::path(current.path()).parent_path();
   }
   fs::path path(suffix);

   for (auto const& item : path) {
      if (item == ".") {
      } else if (item == "..") {
         newpath = newpath.parent_path();
      } else {
         newpath = newpath / item;
      }
   }

   network::uri::uri newuri;
   newuri << network::uri::scheme(current.scheme())
          << network::uri::host(current.host())
          << network::uri::path(newpath.generic_string());

   return newuri.string();
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

AnimationPtr ResourceManager2::LoadAnimation(network::uri::uri const& canonical_uri) const
{
   std::ifstream json_in;
   OpenResource(canonical_uri.string(), json_in);

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
         throw InvalidResourceException(canonical_uri, "'type' field missing");
      }
      if (type != "animation") {
         throw InvalidResourceException(canonical_uri, "node type is not 'animation'");
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

JSONNode const& ResourceManager2::LookupJson(std::string uri) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   network::uri::uri canonical_uri = ConvertToCanonicalUri(uri, ".txt");
   std::string key = canonical_uri.string();

   auto i = jsons_.find(key);
   if (i != jsons_.end()) {
      return i->second;
   }
   return (jsons_[key] = LoadJson(canonical_uri));
}

AnimationPtr ResourceManager2::LookupAnimation(std::string uri) const
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);

   network::uri::uri canonical_uri = ConvertToCanonicalUri(uri, ".txt");   
   std::string key = canonical_uri.string();

   auto i = animations_.find(key);
   if (i != animations_.end()) {
      return i->second;
   }
   return (animations_[key] = LoadAnimation(canonical_uri));
}

void ResourceManager2::OpenResource(std::string const& canonical_uri, std::ifstream& in) const
{
   std::string filepath = GetFilepathForUri(canonical_uri);

   in = std::ifstream(filepath, std::ios::in | std::ios::binary);
   if (!in.good()) {
      // how can we get here?  GetFilepathForUri validates the filepath.
      // maybe if we can't open the file for some reason?
      throw InvalidFilePath(filepath);
   }
}

std::string ResourceManager2::GetResourceFileName(std::string const& uri, const char* search_ext) const
{
   return GetFilepathForUri(ConvertToCanonicalUri(uri, search_ext));
}

void ResourceManager2::ParseUriAndFilepath(network::uri::uri const& uri,
                                           network::uri::uri &canonical_uri,
                                           std::string& path,
                                           const char* search_ext) const
{
   if (!uri.is_valid()) {
      throw InvalidUriException(uri);
   }

   fs::path filepath = fs::path(resource_dir_);
   filepath /= uri.host();

   std::string uripath = uri.path();
   boost::char_separator<char> sep("/");
   boost::tokenizer<boost::char_separator<char>> tokens(uripath, sep);

   std::string last_part = uri.host();
   for (auto const& part: tokens) {
      filepath /= part;
      last_part = part;
   }

   // if the file path is a directory, look for a file with the name of
   // the last part in there (e.g. /foo/bar -> /foo/bar/bar.txt
   if (fs::is_directory(filepath) && !last_part.empty()) {
      filepath /= last_part + search_ext;
      canonical_uri = uri.string() + std::string("/") + std::string(last_part) + search_ext;
   } else{
      canonical_uri = uri;
   }

   if (!fs::is_regular_file(filepath)) {
      throw InvalidUriException(uri);
   }

   // xxx: this doesn't work if the path is non-ascii!
   std::wstring native_path = filepath.native();
   path = std::string(native_path.begin(), native_path.end());
}

void ResourceManager2::ConvertToAbsoluteUris(network::uri::uri const& base_uri, JSONNode& node) const
{
   int type = node.type();
   if (type == JSON_NODE || type == JSON_ARRAY) {
      for (auto& i : node) {
         ConvertToAbsoluteUris(base_uri, i);
      }
   } else if (type == JSON_STRING) {
      network::uri::uri absolute_uri = ConvertToAbsoluteUri(base_uri, node.as_string());
      node = JSONNode(node.name(), absolute_uri.string());
   }
}

network::uri::uri ResourceManager2::ConvertToCanonicalUri(network::uri::uri const& uri, const char* search_ext) const
{
   std::string canonical_uri;

   if (!uri.is_valid()) {
      throw InvalidUriException(uri);
   }

   canonical_uri = uri.scheme() + "://";
   canonical_uri += uri.host();

   fs::path filepath = fs::path(resource_dir_);
   filepath /= uri.host();

   std::string uripath = uri.path();
   boost::char_separator<char> sep("/");
   boost::tokenizer<boost::char_separator<char>> tokens(uripath, sep);

   std::string last_part = uri.host();
   for (auto const& part: tokens) {
      filepath /= part;
      last_part = part;
      canonical_uri += std::string("/") + part;
   }

   // if the file path is a directory, look for a file with the name of
   // the last part in there (e.g. /foo/bar -> /foo/bar/bar.txt
   if (fs::is_directory(filepath) && !last_part.empty()) {
      filepath /= last_part + search_ext;
      canonical_uri += std::string("/") + std::string(last_part) + search_ext;
   }

   if (!fs::is_regular_file(filepath)) {
      throw InvalidUriException(uri);
   }
   return canonical_uri;
}


std::string ResourceManager2::GetFilepathForUri(network::uri::uri const& canonical_uri) const
{
   fs::path filepath = fs::path(resource_dir_);
   filepath /= canonical_uri.host();

   std::string uripath = canonical_uri.path();
   boost::char_separator<char> sep("/");
   boost::tokenizer<boost::char_separator<char>> tokens(uripath, sep);

   std::string last_part = canonical_uri.host();
   for (auto const& part: tokens) {
      filepath /= part;
      last_part = part;
   }

   std::wstring native_path = filepath.native();
   return std::string(native_path.begin(), native_path.end());
}

JSONNode ResourceManager2::LoadJson(network::uri::uri const& canonical_uri) const
{
   std::string filepath = GetFilepathForUri(canonical_uri);

   std::ifstream in(filepath, std::ios::in);
   if (!in.good()) {
      throw InvalidUriException(canonical_uri);
   }

   std::stringstream reader;
   reader << in.rdbuf();

   json_string json = reader.str();
   if (!libjson::is_valid(json)) {
      throw InvalidJsonAtUriException(canonical_uri);
   }

   JSONNode node = libjson::parse(json);
   ConvertToAbsoluteUris(canonical_uri, node);
   ParseNodeExtension(canonical_uri, node);

   return node;
}


void ResourceManager2::ParseNodeExtension(network::uri::uri const& uri, JSONNode& node) const
{
   std::string extends = json::get<std::string>(node, "extends", "");
   if (!extends.empty()) {
      network::uri::uri absolute_uri = ConvertToAbsoluteUri(uri, extends);

      JSONNode const& parent = LookupJson(absolute_uri.string());

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