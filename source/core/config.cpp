#include <shlobj.h>
#include "radiant.h"
#include "build_number.h"
#include "config.h"
#include "lib/json/node.h"
#include "libjson.h"
#include <iostream>
#include <rpc.h>
#include <boost/algorithm/string.hpp>
#include "poco/UUIDGenerator.h"

using namespace ::radiant;
using namespace ::radiant::core;

DEFINE_SINGLETON(Config)

//Analytics uses the build_number for a/b testing.
const std::string BUILD_NUMBER = "preview_0.1a";

Config::Config()
{
}

boost::filesystem::path Config::LookupCacheDirectory() const
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK) {
      throw std::invalid_argument("Could not retrieve path to local application data");
   }
   return boost::filesystem::path(path) / GetName();
}

static void JsonParseErrorCallback(json_string const& message)
{
   throw json::InvalidJson(libjson::to_std_string(message).c_str());
}

json::Node Config::ParseJsonFile(boost::filesystem::path const& filepath) const
{
   std::ifstream stream(filepath.string());
   std::stringstream reader;
   reader << stream.rdbuf();
   json_string json_text = reader.str();

   if (!libjson::is_valid(json_text)) {
      std::string error_message = "Unknown reason";

      try {
         JSONNode node = libjson::parse(json_text);
         // force libjson to resolve the nodes and hit the error
         node.write_formatted();
      } catch (std::exception const& e) {
         error_message = e.what();
      }

      throw json::InvalidJson(BUILD_STRING(
         "Invalid JSON: " << error_message << "\n\n" <<
         "in file " << filepath << "\n\n" <<
         "Source:\n" << json_text
      ));
   }

   JSONNode node = libjson::parse(json_text);
   return json::Node(node);
}

void Config::MergeConfigNodes(JSONNode& base, JSONNode const& override) const
{
   if (base.type() != override.type()) {
      throw json::InvalidJson("");
   }

   switch (override.type()) {
   case JSON_NODE:
      {
         for (JSONNode const& n : override) {
            std::string const& name = n.name();
            auto it = base.find(name);
            if (it == base.end()) {
               // override does not exist in base, add it
               base.push_back(n);
            } else {
               // override exists in base. merge the two nodes
               MergeConfigNodes(*it, n);
            }
         }
         break;
      }
   case JSON_ARRAY:
      throw json::InvalidJson(BUILD_STRING("Array not expected on node '" << override.name() << "'"));
      break;
   default:
      base = override;
      break;      
   }
}

json::Node Config::ReadConfigFile(boost::filesystem::path const& file_path, bool throw_on_not_found) const
{
   if (!boost::filesystem::is_regular(file_path)) {
      if (throw_on_not_found) {
         throw std::invalid_argument(BUILD_STRING(file_path << "does not exist"));
      }
      return JSONNode();
   }

   return ParseJsonFile(file_path);
}

void Config::WriteConfigFile(boost::filesystem::path const& file_path, json::Node const& config) const
{
   std::string text = config.write_formatted();
   std::ofstream ostream(file_path.string());
   ostream << text;
}

void Config::CmdLineParamToKeyValue(std::string const& param, std::string& key, std::string& value) const
{
   std::string trimmed_param = param;
   boost::algorithm::trim_left_if(trimmed_param, boost::algorithm::is_any_of("-"));

   size_t const offset = trimmed_param.find('=');
   if (offset != std::string::npos) {
      key = trimmed_param.substr(0, offset);
      value = trimmed_param.substr(offset + 1);
   } else {
      key = std::string();
      value = std::string();
   }
}

json::Node Config::ParseCommandLine(int argc, const char *argv[]) const
{
   json::Node node;
   std::string key, value;

   for (int i=0; i < argc; i++) {
      CmdLineParamToKeyValue(argv[i], key, value);
      if (!key.empty()) {
         node.set(key, value);
      }
   }

   return node;
}   

void Config::Load(int argc, const char *argv[])
{
   json::Node command_line_config = ParseCommandLine(argc, argv);

   // configure filesystem      
   config_filename_ = GetName() + ".json";
   run_directory_ = boost::filesystem::canonical(boost::filesystem::path("."));
   cache_directory_ = LookupCacheDirectory();

   if (!boost::filesystem::is_directory(cache_directory_)) {
      boost::filesystem::create_directory(cache_directory_);
   }

   // read config files
   // need to define JSON_DEBUG in libjson.h to get enable the callback for good error messages
   //libjson::register_debug_callback(JsonParseErrorCallback);
   root_config_node_ = ReadConfigFile(run_directory_ / config_filename_);
   json::Node user_config = ReadConfigFile(cache_directory_ / config_filename_, false);

   // ugly const_cast - alternatively use a friend class or expose a writable node reference
   MergeConfigNodes(const_cast<JSONNode&>(root_config_node_.get_internal_node()), user_config.get_internal_node());
   MergeConfigNodes(const_cast<JSONNode&>(root_config_node_.get_internal_node()), command_line_config.get_internal_node());

   // initialize session
   userid_ = GetProperty("user_id", "");
   if (userid_.empty()) {
      userid_ = Poco::UUIDGenerator::defaultGenerator().create().toString();
      SetProperty("user_id", userid_, true);
   }
   sessionid_ = Poco::UUIDGenerator::defaultGenerator().create().toString();
}

std::string Config::GetName() const
{
   return std::string(PRODUCT_IDENTIFIER);
}

boost::filesystem::path Config::GetCacheDirectory() const
{
   return cache_directory_;
}

boost::filesystem::path Config::GetTmpDirectory() const
{
   return cache_directory_;
}

std::string Config::GetUserID() const
{
   return userid_;
}

std::string Config::GetSessionID() const
{
   return sessionid_;
}

std::string Config::GetBuildNumber() const
{
   return BUILD_NUMBER;
}

bool Config::GetCollectionStatus() const
{
   return GetProperty("collect_analytics", false);
}

void Config::SetCollectionStatus(bool should_collect) 
{
   SetProperty("collect_analytics", should_collect, true);
}

bool Config::IsCollectionStatusSet() const
{
   return HasProperty("collect_analytics");
}
