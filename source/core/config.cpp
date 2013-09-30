#include <shlobj.h>
#include "radiant.h"
#include "config.h"

using namespace ::radiant;
using namespace ::radiant::core;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

DEFINE_SINGLETON(Config)

Config::Config() :
   cmd_line_options_("command line options"),
   config_file_options_("configuration file options")
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

   // Load the config file...
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

