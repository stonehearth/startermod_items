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
#include "build_number.h"
#include "boost/thread.hpp"
#include "lib/crash_reporter/client/crash_reporter_client.h"
#include "csg/random_number_generator.h"

using radiant::client::Application;
//namespace po = boost::program_options;

void protobuf_log_handler(google::protobuf::LogLevel level, const char* filename,
                          int line, const std::string& message)
{
   LOG(INFO) << message;
}

Application::Application()
{
}
   
Application::~Application()
{
}

void Application::LoadConfig(int argc, const char* argv[])
{
   core::Config& config = core::Config::GetInstance();
   config.Load(argc, argv);

   Client::GetInstance().GetConfigOptions();
   game_engine::arbiter::GetInstance().GetConfigOptions();

   crash_dump_uri_ = config.GetProperty("support.crash_dump_server", REPORT_CRASHDUMP_URI);
}

// Returns false on error
bool Application::InitializeCrashReporting(std::string& error_string)
{
   // Don't install exception handling hooks in debug builds
   DEBUG_ONLY(return true;)

   std::string const crash_dump_path = core::Config::GetInstance().GetTmpDirectory().string();
   std::string const userid = core::Config::GetInstance().GetUserID();

   return crash_reporter::client::CrashReporterClient::GetInstance().Start(crash_dump_path, crash_dump_uri_, userid, error_string);
}

boost::asio::ip::tcp::acceptor* Application::FindServerPort()
{            
   boost::asio::ip::tcp::resolver resolver(_io_service);
   csg::RandomNumberGenerator rng;

   for (int i = 0; i < 10; i++)
   {
      // Pick any address after port 10000.
      unsigned short port = rng.GenerateUniformInt(10000, 65535);
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
         LOG(INFO) << "Running Stonehearth server on port " << port;
         server_port_ = port;
         return tcp_acceptor;
      }
   }

   LOG(ERROR) << "Could not find an open port to host Stonehearth!";
   throw std::exception("Could not find an open port to host Stonehearth!");
}

void Application::ClientThreadMain(int server_port)
{
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([=]() {
      radiant::client::Client::GetInstance().run(server_port);
   });
}

int Application::Run(int argc, const char** argv)
{
   // Exception handling prior to initializing crash reporter or log
   try {
      LoadConfig(argc, argv);
   } catch (std::exception const& e) {
      std::string const error_message = BUILD_STRING("Unhandled exception during application bootstrap:\n\n" << e.what());
      crash_reporter::client::CrashReporterClient::TerminateApplicationWithMessage(error_message);
   }

   // Start crash reporter after config has been successfully loaded so we have a temp directory for the dump_path
   // Start before the logger in case the logger fails
   std::string error_string;
   bool crash_reporting_initialized = false;
   try {
      crash_reporting_initialized = InitializeCrashReporting(error_string);
   } catch (...) {
      // Continue running without crash reporter
      // Log the error string when the log comes up
   }

   // Exception handling after crash reporter is initialized
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([&]() {
      core::Config& config = core::Config::GetInstance();

      radiant::logger::init(config.GetTmpDirectory() / (config.GetName() + ".log"));

      // Have to wait for the logger to initialize before logging error
      if (!crash_reporting_initialized) {
         LOG(WARNING) << "Crash reporter failed to start: " << error_string;
      }

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

      game_engine::arbiter::GetInstance().Run(acceptor, &_io_service);
      client_thread.join();

      radiant::logger::exit();
   });

   analytics::StopSession();

   return 0;
}
