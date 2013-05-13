#include <fstream>
#include <sstream>
#include <io.h>
#include <regex>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
#include <boost/tokenizer.hpp>
#include "radiant.h"
#include "res_manager.h"
#include "animation.h"
#include "data_resource.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#include "sha.h"
#include "hex.h"
#include "files.h"

namespace network = ::boost::network;
namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::resources;

// xxx: move helpers like this out to another file...
csg::Region2 resources::ParsePortal(JSONNode const& obj)
{
   csg::Region2 rgn;

   for (const JSONNode& rc : obj["rects"]) {
      rgn += csg::Rect2(csg::Point2(rc[0][0].as_int(), rc[0][1].as_int()),
                        csg::Point2(rc[1][0].as_int() + 1, rc[1][1].as_int() + 1));
   }
   return rgn;
}

std::unique_ptr<ResourceManager2> ResourceManager2::singleton_;

ResourceManager2& ResourceManager2::GetInstance()
{
   if (!singleton_) {
      singleton_.reset(new ResourceManager2());
   }
   return *singleton_;
}

ResourceManager2::ResourceManager2() :
   resource_dir_("./data/")
{
   ASSERT(!singleton_);
}

ResourceManager2::~ResourceManager2()
{
}

#if 0
void ResourceManager2::LoadDirectory(std::string dir)
{
   resource_dir_ = dir;
   fs::recursive_directory_iterator i(dir), end;
   while (i != end) {
      fs::path path(*i++);
      if (fs::is_regular_file(path) && path.extension() == ".txt") {
         LoadJsonFile(path.string());
      }
   }
}
#endif

bool ResourceManager2::LoadJson(network::uri::uri const& uri, JSONNode& node)
{
   ASSERT(uri.is_valid());

   std::ifstream in;
   if (!OpenResource(uri, in)) {
      return false;
   }

   std::stringstream reader;
   reader << in.rdbuf();

   json_string json = reader.str();
   try {
      node = libjson::parse(json);
   } catch (const std::invalid_argument& e) {
      LOG(WARNING) << uri.string() << " is not valid JSON: " << e.what();
      return false;
   }
   return true;
}

const std::shared_ptr<Resource> ResourceManager2::LookupResource(std::string struri)
{
   std::lock_guard<std::mutex> lock(mutex_);

   // If we've already loaded the resources, there's no need to load it
   // again.  Just return the one we've got.
   auto j = resources_.find(struri);
   if (j != resources_.end()) {
      return j->second;
   }

   network::uri::uri uri;
   if (!ParseUri(struri, uri)) {
      LOG(WARNING) << "invalid uri '" << struri << "' in open resource.";
      return nullptr;
   }

   // Load the JSON with the specified id.  This just loads a blob of JSON.
   // We may convert it to something else based on the type.
   JSONNode node;
   if (!LoadJson(uri, node)) {
      return nullptr;
   }

   std::shared_ptr<Resource> resource;

   auto i = node.find("type");
   Resource::ResourceType loadType = Resource::JSON;

   if (i != node.end() && i->type() == JSON_STRING) {
      std::string type = i->as_string();
      if (type == "animation") {
         loadType = Resource::ANIMATION;
      }
   }
   switch (loadType) {
   case Resource::ANIMATION:
      resource = ParseAnimationJsonObject(struri, node);
      break;
   case Resource::JSON:
      ExtendRootJsonNode(uri, node);
      resource = std::make_shared<DataResource>(ExpandJSON(uri, node));
      break;
   }

   if (resource) {
      resources_[struri] = resource;
   }
   return resource;
}

std::shared_ptr<Resource> ResourceManager2::ParseAnimationJsonObject(std::string key, const JSONNode& node)
{
   std::string jsonfile = node.write();
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
      buffer = Animation::JsonToBinary(node);
      std::ofstream out(binfile.string(), std::ios::out | std::ios::binary);
      out << jsonhash << '\0';
      out.write(&buffer[0], buffer.size());
      out.close();
   }
   std::shared_ptr<Animation> animation = std::make_shared<Animation>(key, buffer);

   return animation;
}

std::string ResourceManager2::Checksum(std::string input)
{
   CryptoPP::SHA256 hash;
   byte buffer[2 * CryptoPP::SHA256::DIGESTSIZE]; // Output size of the buffer

   CryptoPP::StringSource(input, true,   // true here means consume all input at once 
      new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(new CryptoPP::ArraySink(buffer, sizeof buffer))));

   return std::string((const char *)buffer, sizeof buffer);
}

void ResourceManager2::ExtendRootJsonNode(network::uri::uri const& uri, JSONNode& node)
{
   auto i = node.find("extends");
   if (i != node.end()) {
      if (i->type() == JSON_STRING) {
         network::uri::uri parentUri;
         std::string parentUriStr = ExpandURL(uri, i->as_string());
         if (!ParseUri(parentUriStr, parentUri)) {
            LOG(WARNING) << "invalid uri '" << parentUri << "' in extend";
            return;
         }         
         parentUriStr = parentUri.string();
         ExtendedNodesMap::iterator j = extendedNodes_.find(parentUriStr);
         if (j == extendedNodes_.end()) {

            if (!LoadJson(network::uri::uri(parentUri), extendedNodes_[parentUriStr])) {
               ASSERT(false); // xxx: throw or something...
            }
            j = extendedNodes_.find(parentUriStr);
            j->second = ExpandJSON(parentUri, j->second);
         }
         LOG(WARNING) << "node pre-extend: " << node.write_formatted();
         LOG(WARNING) << "extending with: " << j->second.write_formatted();
         ExtendNode(node, j->second);
         LOG(WARNING) << "node post-extend: " << node.write_formatted();
      }
      // remove extends...
      node.erase(i);
   }
}

void ResourceManager2::ExtendNode(JSONNode& node, const JSONNode& parent)
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

std::string ResourceManager2::ExpandURL(network::uri::uri const& current, std::string const& str)
{
   if (!boost::starts_with(str, "url(") || !boost::ends_with(str, ")")) {
      return str;
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

JSONNode ResourceManager2::ExpandJSON(network::uri::uri const& uri, JSONNode const& node)
{
   switch (node.type()) {
   case JSON_NODE:
   case JSON_ARRAY: {
      JSONNode result(node.type());
      result.set_name(node.name());
      for (const auto& i : node) {
         result.push_back(ExpandJSON(uri, i));
      }
      return result;
   }
   case JSON_STRING:
      return JSONNode(node.name(), ExpandURL(uri, node.as_string()));
   }
   return node;
}


bool ResourceManager2::ParseUri(std::string const& uristr, network::uri::uri &uri)
{
   uri = uristr;
   if (!uri.is_valid()) {
      LOG(WARNING) << "invalid uri '" << uri << "' in open resource.";
      return false;
   }

   fs::path filepath = fs::path(resource_dir_) / uri.host();
   std::string uripath = uri.path();

   boost::char_separator<char> sep("/");
   boost::tokenizer<boost::char_separator<char>> tokens(uripath, sep);

   std::string last_part;
   for (auto const& part: tokens) {
      filepath /= part;
      last_part = part;
   }

   // if the file path is a directory, look for a file with the name of
   // the last part in there (e.g. /foo/bar -> /foo/bar/bar.txt
   if (fs::is_directory(filepath) && !last_part.empty()) {
      filepath /= last_part + ".txt";
      uri = uristr + std::string("/") + std::string(last_part) + ".txt";
   }
   if (!fs::is_regular_file(filepath)) {
      return false;
   }
   return true;
}


bool ResourceManager2::OpenResource(network::uri::uri const& uri, std::ifstream& in)
{
   LOG(WARNING) << "opening resource '" << uri << "'.";

   if (!uri.is_valid()) {
      LOG(WARNING) << "invalid uri '" << uri << "' in open resource.";
      return false;
   }

   network::uri::uri fileuri;
   std::string str = uri.string();
   if (!ParseUri(str, fileuri)) {
      LOG(WARNING) << "could not open uri '" << str << "' in open resource.";
      return false;
   }

   fs::path filepath = fs::path(resource_dir_) / fileuri.host() / fileuri.path();
   auto fb = filepath.native();
   in.open(filepath.native(), std::ios::in | std::ios::binary);
   return in.good();
}

bool ResourceManager2::OpenResource(std::string const& s, std::ifstream& in)
{
   return OpenResource(network::uri::uri(s), in);
}

