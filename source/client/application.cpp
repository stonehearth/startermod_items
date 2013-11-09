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
#include "poco/UUIDGenerator.h"
#include "boost/thread.hpp"
#include "lib/crash_reporter/client/crash_reporter_client.h"

using radiant::client::Application;

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

bool Application::LoadConfig(int argc, const char* argv[])
{
   core::Config& config = core::Config::GetInstance();

   Client::GetInstance().GetConfigOptions();
   game_engine::arbiter::GetInstance().GetConfigOptions();

   return config.Load(argc, argv);
}

bool Application::InitializeCrashReporting(std::string& error_string)
{
   std::string const crash_dump_path = core::Config::GetInstance().GetTmpDirectory().string();
   std::string const crash_dump_uri = "http://posttestserver.com/post.php"; // 3rd party test server

   return crash_reporter::client::CrashReporterClient::GetInstance().Start(crash_dump_path, crash_dump_uri, error_string);
}

void Application::ClientThreadMain()
{
   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([]() {
      radiant::client::Client::GetInstance().run();
   });
}

int Application::Run(int argc, const char** argv)
{
   core::Config& config = core::Config::GetInstance();

   if (!LoadConfig(argc, argv)) {
      return 0;
   }

   // Start crash reporter after config has been successfully loaded so we have a temp directory for the dump_path
   // Start before the logger in case the logger fails
   std::string error_string;
   bool crash_reporter_started = InitializeCrashReporting(error_string);

   // Not much point in catching exceptions before this point
   // Maybe write a MessageBox with the error

   crash_reporter::client::CrashReporterClient::RunWithExceptionWrapper([&]() {

      radiant::logger::init(config.GetTmpDirectory() / (config.GetName() + ".log"));

      // Have to wait for the logger to initialize before logging error
      if (!crash_reporter_started) {
         LOG(WARNING) << "Crash reporter failed to start: " << error_string;
      }

      // factor this out into some protobuf helper like we did with log
      google::protobuf::SetLogHandler(protobuf_log_handler);

      // Need to load all singletons before spawning threads.
      res::ResourceManager2::GetInstance();
      client::Client::GetInstance();

      //Start the analytics session before spawning threads too
      std::string userid = config.GetUserID();
      std::string sessionid = config.GetSessionID();
      std::string build_number = config.GetBuildNumber();
      analytics::StartSession(userid, sessionid, build_number);

      // IMPORTANT:
      // Breakpad (the crash reporter) doesn't work with std::thread or poco::thread (poco::thread with dynamic linking might work).
      // They do not have the proper top level exception handling behavior which is configured
      // using SetUnhandledExceptionFilter on Windows.
      // Windows CreateThread and boost::thread appear to work
      boost::thread client_thread(ClientThreadMain);

      game_engine::arbiter::GetInstance().Run();
      client_thread.join();

      radiant::logger::exit();
   });

   analytics::StopSession();

   return 0;
}
