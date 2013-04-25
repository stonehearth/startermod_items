#include <fstream>
#include <sstream>
#include <io.h>
#include "radiant.h"
#include "res_manager.h"
#include "number_resource.h"
#include "object_resource.h"
#include "array_resource.h"
#include "string_resource.h"
#include "rig.h"
#include "animation.h"
#include "action.h"
#include "region2d.h"
#include "skeleton.h"
#include "data_resource.h"
#include "boost/filesystem.hpp"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#include "sha.h"
#include "hex.h"
#include "files.h"

namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::resources;

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
}

ResourceManager2::~ResourceManager2()
{
}

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

void ResourceManager2::LoadJsonFile(std::string filename)
{
   std::ifstream in(filename, std::ios::in);
   if (!in.good()) {
      return;
   }

   std::stringstream reader;
   reader << in.rdbuf();
   json_string json = reader.str();
      
   try {
      JSONNode node = libjson::parse(json);
      ParseJsonObject("", node);
   } catch (const std::invalid_argument& e) {
      LOG(WARNING) << filename << " is not valid JSON: " << e.what();
      return;
   }
}

std::shared_ptr<Resource> ResourceManager2::ParseJsonObject(std::string ns, const JSONNode& json)
{
   ASSERT(json.type() == JSON_NODE);

   std::shared_ptr<Resource> obj;
   std::string type = ValidateJsonObject(json);
   if (type == "rig") {
      obj = ParseRigJsonObject(ns, json);
   } else if (type == "action") {
      obj = ParseActionJsonObject(ns, json);
   } else if (type == "animation") {
      obj = ParseAnimationJsonObject(ns, json);
   } else if (type == "region2d") {
      obj = ParseRegion2dJsonObject(ns, json);
   } else if (type == "skeleton") {
      obj = ParseSkeletonJsonObject(ns, json);
   } else if (type == "effect") {
      obj = ParseDataJsonObject(Resource::EFFECT, ns, json);
   } else if (type == "recipe") {
      obj = ParseDataJsonObject(Resource::RECIPE, ns, json);
   } else if (type == "one_of") {
      obj = ParseDataJsonObject(Resource::ONE_OF, ns, json);
   } else if (type == "json") {
      obj = ParseDataJsonObject(Resource::JSON, ns, json);
   } else if (type == "combat_ability") {
      obj = ParseDataJsonObject(Resource::JSON, ns, json);
   } else {
      obj = ParseGenericJsonObject(ns, json);
   }
   return obj;
}

std::shared_ptr<Resource> ResourceManager2::ParseGenericJsonObject(std::string ns, const JSONNode& node)
{
   std::string sep = ns.empty() ? "" : ".";

   auto obj = std::make_shared<ObjectResource>();
   for (const JSONNode& child : node) {
      std::string name = child.name();
      std::string childns = ns + sep + name;

      std::shared_ptr<Resource> value = ParseJson(childns, child);
      (*obj)[name] = value;
      AddResourceToIndex(childns, value);
   }
   return obj;
}

std::shared_ptr<Resource> ResourceManager2::ParseJsonArray(std::string ns, const JSONNode& json)
{
   ASSERT(json.type() == JSON_ARRAY);
   ASSERT(!ns.empty());

   auto obj = std::make_shared<ArrayResource>();
   for (unsigned int i = 0; i < json.size(); i++) {
      std::stringstream childns;
      childns << ns << "[" << i << "]";
      std::shared_ptr<Resource> value = ParseJson(childns.str(), json[i]);
      obj->Append(value);
      AddResourceToIndex(childns.str(), value);
   }
   return obj;
}

std::shared_ptr<Resource> ResourceManager2::ParseJson(std::string ns, const JSONNode& node)
{
   if (node.type() == JSON_NODE) {
      return ParseJsonObject(ns, node);
   } else if (node.type() == JSON_ARRAY) {
      return ParseJsonArray(ns, node);
   } else if (node.type() == JSON_NUMBER) {
      return std::make_shared<NumberResource>(node.as_float());
   } else if (node.type() == JSON_STRING) {
      return std::make_shared<StringResource>(node.as_string());
   }
   ASSERT(false);
   return nullptr;
}

void ResourceManager2::AddResourceToIndex(std::string key, std::shared_ptr<Resource> value)
{
   LOG(WARNING) << "Adding resource " << key << " to database.";
   ASSERT(!resources_[key]);
   resources_[key] = value;
}

std::string ResourceManager2::ValidateJsonObject(const JSONNode& json)
{
   ASSERT(json.type() == JSON_NODE);
   auto i = json.find("type");
   if (i != json.end()) {
      return i->as_string();
   }
   return "";
}

const std::shared_ptr<Resource> ResourceManager2::Lookup(std::string id) const
{
   std::lock_guard<std::mutex> lock(mutex_);

   auto i = resources_.find(id);
   return i == resources_.end() ? nullptr : i->second;
}

std::shared_ptr<Resource> ResourceManager2::ParseRigJsonObject(std::string ns, const JSONNode& json)
{
   std::shared_ptr<Rig> rig = std::make_shared<Rig>(ns);

   const JSONNode& parts = json["parts"];
   ASSERT(parts.type() == JSON_NODE);

   for (const JSONNode& part : parts) {
      std::string bone = part.name();
      std::string model = part.as_string();
      rig->AddModel(bone, Model(model, "", "", math3d::point3::origin));
   }

   return rig;
}

std::shared_ptr<Resource> ResourceManager2::ParseRegion2dJsonObject(std::string ns, const JSONNode& json)
{
   std::shared_ptr<Region2d> region = std::make_shared<Region2d>();

   const JSONNode& rects = json["rects"];
   ASSERT(rects.type() == JSON_ARRAY);

   csg::Region2& rgn = region->ModifyRegion();
   for (const JSONNode& rc : rects) {
      rgn += csg::Rect2(csg::Point2(rc[0][0].as_int(), rc[0][1].as_int()),
                        csg::Point2(rc[1][0].as_int() + 1, rc[1][1].as_int() + 1));
   }

   return region;
}

std::shared_ptr<Resource> ResourceManager2::ParseSkeletonJsonObject(std::string ns, const JSONNode& json)
{
   std::shared_ptr<Skeleton> skeleton = std::make_shared<Skeleton>(json);
   return skeleton;
}

std::shared_ptr<Resource> ResourceManager2::ParseDataJsonObject(Resource::ResourceType type, std::string ns, const JSONNode& json)
{
   return std::make_shared<DataResource>(type, json);
}

std::shared_ptr<Resource> ResourceManager2::ParseActionJsonObject(std::string ns, const JSONNode& json)
{
   return std::make_shared<Action>(ns, json);
}

std::shared_ptr<Resource> ResourceManager2::ParseAnimationJsonObject(std::string key, const JSONNode& node)
{
   std::string jsonfile = node.write();
   std::string jsonhash = Checksum(jsonfile);
   fs::path binfile = fs::path(resource_dir_) / (key + std::string(".bin"));

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

