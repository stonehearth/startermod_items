#include "pch.h"
#include "radiant_logger.h"
#include "radiant_macros.h"
#include "lib/json/node.h"
#include "client.h" 
#include "application.h"
#include "resources/res_manager.h"
#include "lib/perfmon/perfmon.h"
#include "lib/analytics/analytics.h"
#include "core/config.h"
#include "core/system.h"
#include "build_number.h"
#include "boost/thread.hpp"
#include "lib/crash_reporter/client/crash_reporter_client.h"
#include "csg/random_number_generator.h"
#include "lib/json/json.h"

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

void protobuf_log_handler(google::protobuf::LogLevel level, const char* filename,
                          int line, const std::string& message)
{
   LOG(network, 1) << message;
}

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

int Application::Run(int argc, const char** argv)
{
   // Exception handling prior to initializing crash reporter
   try {
      if (ShouldShowHelp(argc, argv)) {
         ShowHelp();
         std::exit(0);
      }
      radiant::logger::Init(core::System::GetInstance().GetTempDirectory() / LOG_FILENAME);
      json::InitialzeErrorHandler();
      core::Config::GetInstance().Load(argc, argv);
      radiant::logger::InitLogLevels();
   } catch (std::exception const& e) {
      std::string const error_message = BUILD_STRING("Error starting application:\n\n" << e.what());
      try {
         // Log it if possible
         LOG_CRITICAL() << error_message;
      } catch (...) {}
      crash_reporter::client::CrashReporterClient::TerminateApplicationWithMessage(error_message);
   }

   try {
      InitializeCrashReporting();
   } catch (std::exception const& e) {
      // Continue running without crash reporter
      LOG_CRITICAL() << "Crash reporter failed to start: " << e.what();
   }

   // Exception handling after initializing crash reporter
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([&]() {
      core::Config& config = core::Config::GetInstance();

      // factor this out into some protobuf helper like we did with log
      google::protobuf::SetLogHandler(protobuf_log_handler);

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

      sim_.reset(new simulation::Simulation());
      sim_->Run(acceptor, &_io_service);
      client_thread.join();

      radiant::logger::Exit();
   });

   analytics::StopSession();

   return 0;
}
