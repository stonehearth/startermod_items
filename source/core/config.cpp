#include <shlobj.h>
#include "radiant.h"
#include "config.h"
#include <iostream>
#include <rpc.h>
#pragma comment(lib, "rpcrt4")
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace ::radiant;
using namespace ::radiant::core;

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::property_tree;

DEFINE_SINGLETON(Config)

Config::Config() :
   cmd_line_options_("command line options"),
   config_file_options_("configuration file options"),
   sessionid_(""),
   userid_("")
{
}

bool Config::Load(std::string const& name, int argc, const char *argv[])
{
   name_ = name;

   cmd_line_options_.add_options()
      ("help",             "produce help message")
      ("cache.directory",  po::value<std::string>(),
                           "the directory containing the runtime game files")
      ("config",           po::value<std::string>(&config_filename_)->default_value(name + ".ini"),
                           "name of a file of a configuration.")
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

   //Make sure we have a userid
   userid_ = GetUserID();

   // Load the config file...
   LoadConfigFile(run_directory_ / config_filename_);
   
   //Help! Commented out for now because currently the config file
   //doesn't contain any data except userid and reading through
   //configvm_ conflicts with the stuff that got written in as the ini file.
   //LoadConfigFile(cache_directory_ / config_filename_);

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
   return name_;
}

boost::filesystem::path Config::GetCacheDirectory() const
{
   return cache_directory_;
}

boost::filesystem::path Config::GetTmpDirectory() const
{
   return cache_directory_;
}

//GetUserID
//Return userid or makes one (and enclosing file, if necessary) and then returns it.
//Note that write_ini will create a new file if necessary. It will also intelligently 
//not overwrite any values already in the file.
//Tested with file empty, file present, file present but no userid data, and file present
//with junk data but no userid data.
//Reference: http://stackoverflow.com/questions/15647299/how-to-read-and-write-ini-files-using-boost-library
//Reference: http://www.boost.org/doc/libs/1_53_0/doc/html/boost_propertytree/accessing.html
//Reference: http://www.boost.org/doc/libs/1_44_0/doc/html/boost_propertytree/tutorial.html
std::string Config::GetUserID()
{
   if (userid_ == "") {
      //userid has not yet been set in this instance of the program. Try to load it from the ini file. 
      fs::path save_path = cache_directory_ / config_filename_;
      std::string save_path_str = save_path.string();
      pt::ptree properties;

      if (fs::is_regular(cache_directory_ / config_filename_)) {
         //Ok, the file exists. Load it into a property tree
         pt::read_ini(save_path_str, properties);
         std::string tempID = properties.get("userid", "");
         
         if (tempID != "") {
            //Success!
            userid_ = tempID;   
         } else {
            //The file exists but the key isn't there.  
            userid_ = MakeUUIDString();
            properties.put("userid", userid_);
            pt::write_ini(save_path_str, properties);
         }

      } else {
         //There is no file.  Make the id and save it out. 
         userid_ = MakeUUIDString();
         properties.put("userid", userid_);
         pt::write_ini(save_path_str, properties);
      }

   }
   return userid_;
}

//Returns the session ID if it exists, and makes a new one for the session if it doesn't.
std::string Config::GetSessionID()
{
   if (sessionid_ == "") {
      return MakeUUIDString();
   } else {
      return sessionid_;
   }
}

//Make a new UUID and return it as a string.
//the UUID to string function seems to internally
//allocate memory, so we have to remember to free it
std::string Config::MakeUUIDString() 
{
   std::string resultString;
   unsigned char* sTemp;
   UUID* uuid = new UUID;
   HRESULT hr;
   hr = UuidCreate(uuid);
   if (hr == RPC_S_OK) {
      hr = UuidToString(uuid, &sTemp);
      if (hr == RPC_S_OK && sTemp != NULL) {
         //N00b alert! I'm coercing the unsigned char* into a char*; is this OK?
         std::string tempStr(reinterpret_cast<const char *>(sTemp), strlen(reinterpret_cast<const char *>(sTemp)));
         resultString = tempStr;
         RpcStringFree(&sTemp);
      }
   }
   delete uuid;
   uuid = NULL;

   return resultString;
}

