#include "pch.h"
#include "client.h" 
#include "application.h"
#include "resources/res_manager.h"
#include "httpd/server.h"
#include <thread>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using radiant::client::Application;

po::variables_map configvm;

Application::Application()
{
}
   
Application::~Application()
{
}

bool Application::LoadConfig(int argc, const char** argv)
{
   std::string configFile;

   // These options are only allowed on the command line
   po::options_description cmdLineOptions("Generic options");
   cmdLineOptions.add_options()
      ("help",    "produce help message")
      ("config",  po::value<std::string>(&configFile)->default_value("stonehearth.ini"),
                  "name of a file of a configuration.")
      ;

   // These options are allowed on the command line or the config file.
   po::options_description configOptions("Configuration");

   configOptions.add_options()
      ("ui.docroot", po::value<std::string>()->default_value("docroot"), "the document root for the ui")
      ;
   Client::GetInstance().GetConfigOptions(configOptions);
   game_engine::arbiter::GetInstance().GetConfigOptions(configOptions);

   po::options_description visible("Allowed options");
   visible.add(cmdLineOptions).add(configOptions);

   po::options_description config_file_options;
   config_file_options.add(configOptions);

   // fun fun...
   po::positional_options_description p;
   p.add("input-file", -1);

   auto options = po::command_line_parser(argc, argv)
      //.options(cmdLineOptions)
      .options(visible)
      .positional(p)
      .allow_unregistered()
      .run();

   po::store(options, configvm);
   po::notify(configvm);

   if (configvm.count("help")) {
      std::cout << visible << std::endl;
      return false;
   }

   // Load the config file...
   std::ifstream ifs(configFile.c_str());
   if (ifs) {
      po::store(po::parse_config_file(ifs, config_file_options), configvm);
      po::notify(configvm);
   }

   // Override with the per-user config file...
   char *appdata = getenv("APPDATA");
   if (appdata) {
      auto userConfigFile = fs::path(appdata) / "Stonehearth" / "Stonehearth.ini";
      std::ifstream ifs(userConfigFile.c_str());
      if (ifs) {
         po::store(po::parse_config_file(ifs, config_file_options), configvm);
         po::notify(configvm);
      }
   }
   

   return true;
}

int Application::run(lua_State* L, int argc, const char** argv)
{
   if (!LoadConfig(argc, argv)) {
      return 0;
   }
   return Start(L);
}

int Application::Start(lua_State* L)
{
   httpd::Server server;

   const char *docroot = configvm["ui.docroot"].as<std::string>().c_str();
   const char *port = "1336";
   
   std::thread client([&]() {
      Client::GetInstance().run();
   });
   server.Start(docroot, port);

   game_engine::arbiter::GetInstance().Run(L);
   client.join();

   server.Stop();

   return 0;
}
