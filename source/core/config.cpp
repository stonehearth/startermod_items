#include <shlobj.h>
#include "radiant.h"
#include "build_number.h"
#include "config.h"
#include "radiant_exceptions.h"
#include "lib/json/node.h"
#include "lib/json/json.h"
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

Config::Config() : config_filename_(GetName() + ".json")
{
}

boost::filesystem::path Config::DefaultCacheDirectory() const
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK) {
      throw std::invalid_argument("Could not retrieve path to local application data");
   }
   return boost::filesystem::path(path) / GetName();
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
      return json::Node();
   }

   JSONNode node;
   std::string error_message;
   bool success;

   success = json::ReadJsonFile(file_path.string(), node, error_message);
   if (!success) {
      throw core::Exception(BUILD_STRING("Error reading file " << file_path << ":\n\n" << error_message));
   }
      
   return json::Node(node);
}

void Config::WriteConfigFile(boost::filesystem::path const& file_path, json::Node const& config) const
{
   std::string text = config.write_formatted();
   std::ofstream ostream(file_path.string());
   ostream << text;
}

void Config::CmdLineOptionToKeyValue(std::string const& param, std::string& key, std::string& value) const
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
      CmdLineOptionToKeyValue(argv[i], key, value);
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
   run_directory_ = boost::filesystem::canonical(boost::filesystem::path("."));
   cache_directory_ = command_line_config.get("cache_directory", DefaultCacheDirectory().string());

   if (!boost::filesystem::is_directory(cache_directory_)) {
      try {
         boost::filesystem::create_directory(cache_directory_);
      } catch (std::exception const& e) {
         throw core::Exception(BUILD_STRING("Cannot create cache directory " << cache_directory_ << ":\n\n" << e.what()));
      }
   }

   // read config files
   base_config_filename_ = run_directory_ / config_filename_;
   override_config_filename_ = cache_directory_ / config_filename_;
   JSONNode internal_root_config = ReadConfigFile(base_config_filename_).get_internal_node();
   json::Node user_config = ReadConfigFile(override_config_filename_, false);

   // user config overrides base config
   try {
      // Merge the root config before wrapping it as a json::Node because json::Node cannot expose it without copying
      MergeConfigNodes(internal_root_config, user_config.get_internal_node());
   } catch (std::exception const& e) {
      throw core::Exception(BUILD_STRING("Error merging " << override_config_filename_ << " with " << base_config_filename_ << ":\n\n" << e.what()));
   }

   // command line options override options in config files
   try {
      // Merge the root config before wrapping it as a json::Node because json::Node cannot expose it without copying
      MergeConfigNodes(internal_root_config, command_line_config.get_internal_node());
   } catch (std::exception const& e) {
      throw core::Exception(BUILD_STRING("Error merging command line options with " << base_config_filename_ << ":\n\n" << e.what()));
   }

   root_config_ = json::Node(internal_root_config);

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

boost::filesystem::path Config::GetTempDirectory() const
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
