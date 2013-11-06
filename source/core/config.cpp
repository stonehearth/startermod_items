#include <shlobj.h>
#include "radiant.h"
#include "build_number.h"
#include "config.h"
#include <iostream>
#include <rpc.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace ::radiant;
using namespace ::radiant::core;

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::property_tree;

DEFINE_SINGLETON(Config)

//Analytics uses the build_number for a/b testing.
const std::string BUILD_NUMBER = "preview_0.1a";

Config::Config() :
   cmd_line_options_("command line options"),
   config_file_options_("configuration file options"), 
   collect_analytics_("yes")
{
}

bool Config::Load(int argc, const char *argv[])
{
   std::string name = GetName();

   cmd_line_options_.add_options()
      ("help",             "produce help message")
      ("cache.directory",  po::value<std::string>(),
                           "the directory containing the runtime game files")
      ("config",           po::value<std::string>(&config_filename_)->default_value(name + ".ini"),
                           "name of a file of a configuration.")
      ;

   config_file_options_.add_options()
      ("userid", po::value<std::string>())
      ("collect_analytics", po::value<std::string>())
      ;

   auto options = po::command_line_parser(argc, argv)
      //.options(cmdLineOptions)
      .options(cmd_line_options_)
      .allow_unregistered()
      .run();

   po::store(options, configvm_);
   po::notify(configvm_);

   // If they just asked for help, bail now...
   if (configvm_.count("help")) {
      std::cout << cmd_line_options_ << std::endl;
      return false;
   }

   // Figure out where to keep temporary files
   if (configvm_.count("cache.directory")) {
      cache_directory_ = configvm_["cache.directory"].as<std::string>();
   } else {
      TCHAR path[MAX_PATH];
      if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK) {
         throw std::invalid_argument("could not retrieve path to common application data");
      }
      cache_directory_ = fs::path(path) / name;
   }
   if (!fs::is_directory(cache_directory_)) {
      fs::create_directory(cache_directory_);
   }

   // Figure out where to run the game...
   run_directory_ = boost::filesystem::path(".");
   if (!fs::is_directory(run_directory_)) {
      throw std::invalid_argument(BUILD_STRING("run directory " << run_directory_ << " does not exist."));
   }

   // If there's no .ini file here, complain quite loudly...
   if (!fs::is_regular(run_directory_ / config_filename_)) {
      throw std::invalid_argument(BUILD_STRING("run directory " << run_directory_ << " does not contain " << name << " data."));
   }

   //Make sure we have a session and userid
   userid_ = ReadConfigOption("userid");
   if (userid_.empty()) {
      userid_ = MakeUUIDString();
      WriteConfigOption("userid", userid_);
   }
   sessionid_ = MakeUUIDString();
   collect_analytics_ = ReadConfigOption("collect_analytics");

   // Load the config files...
   LoadConfigFile(run_directory_ / config_filename_);
   LoadConfigFile(cache_directory_ / config_filename_);

   return true;
}

void Config::LoadConfigFile(boost::filesystem::path const& configfile)
{
   if (fs::is_regular(configfile) || fs::is_symlink(configfile)) {
      std::ifstream ifs(configfile.string());
      if (ifs) {
         po::store(po::parse_config_file(ifs, config_file_options_), configvm_);
         po::notify(configvm_);
      }
   }
}

boost::program_options::options_description& Config::GetConfigFileOptions() 
{
   return config_file_options_;
}

boost::program_options::options_description& Config::GetCommandLineOptions() 
{
   return cmd_line_options_;
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

std::string Config::GetUserID()
{
   return userid_;
}

//True if we should collect analytics
//(if collect_analytics_ is "yes") false otherwise
bool Config::GetCollectionStatus()
{
   return (collect_analytics_ == "yes");
}

void Config::SetCollectionStatus(bool should_collect) 
{
   if (should_collect) {
      collect_analytics_ = "yes";
   } else {
      collect_analytics_ = "no";
   }
   WriteConfigOption("collect_analytics", collect_analytics_);
}

//True if the user has set the collecton status, false otherwise
bool Config::IsCollectionStatusSet()
{
   return (!collect_analytics_.empty());
}

//WriteConfigOption
//Note that write_ini will create a new file if necessary. It will also intelligently 
//not overwrite any values already in the file.
//Tested with file empty, file present, file present but no userid data, and file present
//with junk data but no userid data.
void Config::WriteConfigOption(std::string option_name, std::string option_value)
{
    //this option has not yet been set in this instance of the program. Try to load it from the ini file.
   fs::path save_path = cache_directory_ / config_filename_;
   std::string save_path_str = save_path.string();
   pt::ptree properties;

   properties.put(option_name, option_value);
   pt::write_ini(save_path_str, properties);
}

//ReadConfigOption
//Return specified option or "" if the file is not there
//Reference: http://stackoverflow.com/questions/15647299/how-to-read-and-write-ini-files-using-boost-library
//Reference: http://www.boost.org/doc/libs/1_53_0/doc/html/boost_propertytree/accessing.html
//Reference: http://www.boost.org/doc/libs/1_44_0/doc/html/boost_propertytree/tutorial.html
std::string Config::ReadConfigOption(std::string option_name)
{
   //this option has not yet been set in this instance of the program. Try to load it from the ini file.
   fs::path save_path = cache_directory_ / config_filename_;
   std::string save_path_str = save_path.string();
   pt::ptree properties;

   if (fs::is_regular(save_path)) {
      //Ok, the file exists. Load it into a property tree
      pt::read_ini(save_path_str, properties);
      std::string new_value = properties.get(option_name, "");

      if (!new_value.empty()) {
         //we got the id, can just return it now
         return new_value;
      }
   }

   //If we're here, either the file doesn't exist, or the file exists
   //but the key isn't there.
   return "";
}

std::string Config::GetSessionID()
{
   return sessionid_;
}

std::string Config::GetBuildNumber()
{
   return BUILD_NUMBER;
}

//Make a new UUID and return it as a string.
//the UUID to string function seems to internally
//allocate memory, so we have to remember to free it
std::string Config::MakeUUIDString() 
{
   std::string resultString;
   char* sTemp;
   UUID uuid;
   HRESULT hr;
   hr = UuidCreate(&uuid);
   if (hr == RPC_S_OK) {
      hr = UuidToString(&uuid, (unsigned char**) &sTemp);
      if (hr == RPC_S_OK && sTemp != NULL) {
         std::string tempStr(sTemp, strlen(sTemp));         
         resultString = tempStr;
         RpcStringFree((unsigned char**) &sTemp);
      } else {
         LOG(WARNING) << "uuid creation error: " << hr;
      }
   }

   return resultString;
}

