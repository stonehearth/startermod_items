#include "pch.h"
#include "radiant_logger.h"
#include "radiant_macros.h"
#include "lib/json/node.h"
#include "client.h" 
#include "application.h"
#include "resources/res_manager.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/timer.h"
#include "lib/analytics/analytics.h"
#include "core/config.h"
#include "core/system.h"
#include "build_number.h"
#include "boost/thread.hpp"
#include "lib/crash_reporter/client/crash_reporter_client.h"
#include "csg/random_number_generator.h"
#include "lib/json/json.h"
#include "type_registry/type_registry.h"
#include <google/protobuf/stubs/common.h>

using namespace radiant;
using radiant::client::Application;

#define APP_LOG(level)     LOG(app, level)

static std::string const LOG_FILENAME = std::string(PRODUCT_IDENTIFIER) + ".log";
static std::string const EXE_NAME = std::string(PRODUCT_IDENTIFIER) + ".exe";
static std::string const HELP_MESSAGE = BUILD_STRING(
   "Options can be specified on the command-line using the format:\n" <<
   "      " << EXE_NAME << " name1=value1 name2=value2 name3=value3\n\n" <<
   "Name should be the path to the property you want to set. For example, to set this config file property:\n" <<
   "      \"renderer\" : { \"enable_shadows\" }\n" <<
   "you would use:\n" <<
   "      " << EXE_NAME << " renderer.enable_shadows=true\n\n" <<
   "Any of the options specified in the stonehearth.json config files can be set this way and will override the config file setting.\n\n" <<
   "To show this message again use:\n" <<
   "      " << EXE_NAME << " help"
);

Application::Application()
{
}
   
Application::~Application()
{
}

// Returns false on error
void Application::InitializeCrashReporting()
{
   // Don't install exception handling hooks in debug builds
   DEBUG_ONLY(return;)

   core::Config& config = core::Config::GetInstance();

   if (config.Get("enable_crash_dump_server", true)) {
      std::string const crash_dump_path = core::System::GetInstance().GetTempDirectory().string();
      std::string const userid = config.GetUserID();
      crash_dump_uri_ = config.Get("crash_dump_server", REPORT_CRASHDUMP_URI);

      crash_reporter::client::CrashReporterClient::GetInstance().Start(crash_dump_path, crash_dump_uri_, userid);
   }
}

boost::asio::ip::tcp::acceptor* Application::FindServerPort()
{            
   boost::asio::ip::tcp::resolver resolver(_io_service);
   csg::RandomNumberGenerator rng;

   for (int i = 0; i < 10; i++)
   {
      // Pick any address after port 10000.
      unsigned short port = rng.GetInt(10000, 65535);
      boost::asio::ip::tcp::resolver::query query("127.0.0.1", std::to_string(port));
      boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
      boost::asio::ip::tcp::acceptor* tcp_acceptor;

      tcp_acceptor = new boost::asio::ip::tcp::acceptor(_io_service);

      tcp_acceptor->open(endpoint.protocol());
      tcp_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
      
      try {
         tcp_acceptor->bind(endpoint);
      } catch(...) {
         delete tcp_acceptor;
         tcp_acceptor = nullptr;
      }

      if (tcp_acceptor != nullptr)
      {
         APP_LOG(1) << "Running Stonehearth server on port " << port;
         server_port_ = port;
         return tcp_acceptor;
      }
   }

   LOG_CRITICAL() << "Could not find an open port to host Stonehearth!";
   throw std::exception("Could not find an open port to host Stonehearth!");
}

void Application::ClientThreadMain(int server_port)
{
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([=]() {
      radiant::log::SetCurrentThreadName("client");
      radiant::client::Client::GetInstance().run(server_port);
   });
}

bool Application::ShouldShowHelp(int argc, const char* argv[]) const
{
   if (argc > 1) {
      std::string option(argv[1]);
      return option.find('=') == std::string::npos;
   }
   return false;
}

void Application::ShowHelp() const
{
   // Should call a cross-platform message window
   MessageBox(nullptr, HELP_MESSAGE.c_str(), PRODUCT_IDENTIFIER, MB_OK);
}

bool Application::ShouldRelaunch64Bit() const
{
   if (!core::System::IsPlatform64Bit()) {
      return false;
   }
   if (core::System::IsProcess64Bit()) {
      return false;
   }
   return core::Config::GetInstance().Get<bool>("enable_64_bit", true);
 }

int Application::Run(int argc, const char** argv)
{
   // Exception handling prior to initializing crash reporter
   try {
      // Some people are double-clicking on the Stonehearth.exe in the x64 directory
      // manually trying to run the 64-bit build.  This will end up throwing a "cannot
      // find settings.json" file since we expect the working directory to be one level
      // up.  If we're a 64-bit build and we detect that our current directory is in the
      // x64 dir, just cd up one level before trying to initialize anything.
      if (core::System::IsProcess64Bit()) {
         TCHAR tcurrent[MAX_PATH];
         if (GetCurrentDirectory(MAX_PATH - 1, tcurrent)) {
            std::string current(tcurrent);
            if (boost::ends_with(current, "\\X64") || boost::ends_with(current, "\\X64\\") ||
                boost::ends_with(current, "\\x64") || boost::ends_with(current, "\\x64\\")) {
               std::string uplevel = current.substr(0, current.size() - 4);
               SetCurrentDirectory(uplevel.c_str());
            }
         }
      }

      if (ShouldShowHelp(argc, argv)) {
         ShowHelp();
         std::exit(0);
      }

      TypeRegistry::Initialize();
      json::InitialzeErrorHandler();
      core::Config::GetInstance().Load(argc, argv);

      if (ShouldRelaunch64Bit()) {
         std::string binary = core::Config::GetInstance().Get<std::string>("x64_binary", "x64\\Stonehearth.exe");
         core::Process p(binary, GetCommandLine());
         if (p.IsRunning()) {
            p.Detach();
            return 0;
         }
      }

      bool show_console = false;
      DEBUG_ONLY(show_console = true;)
      if (core::Config::GetInstance().Get<bool>("logging.show_console", show_console)) {
         radiant::log::InitConsole();
      }
      radiant::log::Init(core::System::GetInstance().GetTempDirectory() / LOG_FILENAME);
      radiant::log::InitLogLevels();
      radiant::log::SetCurrentThreadName("server");

      google::protobuf::SetLogHandler([](google::protobuf::LogLevel level, const char* filename, int line, std::string const& message) {
         LOG(protobuf, 0) << " " << message;
      });
      APP_LOG(1) << "Stonehearth Version " << PRODUCT_FILE_VERSION_STR << " (" << (core::System::IsProcess64Bit() ? "x64" : "x32") << ")";

      perfmon::Timer_Init();
   } catch (std::exception const& e) {
      std::string const error_message = BUILD_STRING("Error starting application:\n\n" << e.what());
      try {
         // Log it if possible
         LOG_CRITICAL() << error_message;
      } catch (...) {}
      crash_reporter::client::CrashReporterClient::TerminateApplicationWithMessage(error_message);
   }

   LOG_(0) << "Initializing crash reporter";
   try {
      InitializeCrashReporting();
   } catch (std::exception const& e) {
      // Continue running without crash reporter
      LOG_CRITICAL() << "Crash reporter failed to start: " << e.what();
   }

   // Exception handling after initializing crash reporter
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([&]() {
      core::Config& config = core::Config::GetInstance();

      lua::Initialize(config.Get<bool>("enable_lua_jit", true));

      // Need to load all singletons before spawning threads.
      res::ResourceManager2::GetInstance();
      client::Client::GetInstance();

      // Find a port for our server.
      boost::asio::ip::tcp::acceptor* acceptor = FindServerPort();

      // Start the analytics session before spawning threads too
      std::string userid = config.GetUserID();
      std::string sessionid = config.GetSessionID();
      std::string build_number = config.GetBuildNumber();
      analytics::StartSession(userid, sessionid, build_number);

      // IMPORTANT:
      // Breakpad (the crash reporter) doesn't work with std::thread or poco::thread (poco::thread with dynamic linking might work).
      // They do not have the proper top level exception handling behavior which is configured
      // using SetUnhandledExceptionFilter on Windows.
      // Windows CreateThread and boost::thread appear to work
      boost::thread client_thread(ClientThreadMain, server_port_);

      sim_.reset(new simulation::Simulation(PRODUCT_FILE_VERSION_STR));
      sim_->Run(acceptor, &_io_service);
      client_thread.join();

      radiant::log::Exit();
   });

   analytics::StopSession();

   return 0;
}
