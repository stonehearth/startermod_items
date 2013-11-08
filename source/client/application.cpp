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

// Uncomment if assertions should be handled by Breakpad in DEBUG builds
#define ENABLE_BREAKPAD_IN_DEBUG_BUILDS

using radiant::client::Application;

static std::string const NAMED_PIPE_PREFIX = "\\\\.\\pipe\\";
static std::string const CRASH_REPORTER_NAME = "crash_reporter";

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

std::string Application::GeneratePipeName() {
   std::string const uuid_string = Poco::UUIDGenerator::defaultGenerator().create().toString();
   std::string const pipe_name = BUILD_STRING(NAMED_PIPE_PREFIX << CRASH_REPORTER_NAME << "\\" << uuid_string);
   return pipe_name;
}

void Application::StartCrashReporter(std::string const& crash_dump_path, std::string const& crash_dump_pipe_name, std::string const& crash_dump_uri)
{
   std::string const filename = CRASH_REPORTER_NAME + ".exe";
   std::string const command_line = BUILD_STRING(filename << " " << crash_dump_pipe_name << " " << crash_dump_path << " " << crash_dump_uri);

   crash_reporter_process_.reset(new radiant::core::Process(command_line));
}

void Application::InitializeExceptionHandlingEnvironment(std::string const& crash_dump_pipe_name)
{
#ifdef ENABLE_BREAKPAD_IN_DEBUG_BUILDS
   //_CrtSetReportMode(_CRT_ASSERT, 0);
#endif

   std::wstring pipe_name_wstring(crash_dump_pipe_name.begin(), crash_dump_pipe_name.end());

   // This API is inconsistent, but trying to avoid too many changes to Breakpad's sample code in case it versions
   exception_handler_.reset(new google_breakpad::ExceptionHandler(
                                                 std::wstring(),                   // local dump path (ignored since we are out of process)
                                                 nullptr,                          // filter callback 
                                                 nullptr,                          // minidump callback
                                                 nullptr,                          // context for the callbacks
                                                 google_breakpad::ExceptionHandler::HANDLER_ALL, // exceptions to trap
                                                 MiniDumpNormal,                   // type of minidump
                                                 pipe_name_wstring.data(),         // name of pipe for out of process dump
                                                 nullptr));                        // CustomClientInfo
}

void Application::InitializeCrashReporting()
{
   std::string const crash_dump_path = core::Config::GetInstance().GetTmpDirectory().string();
   std::string const crash_dump_pipe_name = GeneratePipeName();
   std::string const crash_dump_uri = "http://posttestserver.com/post.php"; // 3rd party test server 

   StartCrashReporter(crash_dump_path, crash_dump_pipe_name, crash_dump_uri);
   InitializeExceptionHandlingEnvironment(crash_dump_pipe_name);
}

void Application::ClientThreadMain()
{
#ifdef _DEBUG
   try {
#endif // _DEBUG
      radiant::client::Client::GetInstance().run();
#ifdef _DEBUG
   } catch (std::exception &e) {
      LOG(WARNING) << "unhandled exception: " << e.what();
      ASSERT(false);
   }
#endif // _DEBUG
}

int Application::Run(int argc, const char** argv)
{
#ifdef _DEBUG
   try {
#endif // _DEBUG
      core::Config& config = core::Config::GetInstance();

      if (!LoadConfig(argc, argv)) {
         return 0;
      }

      std::string error_reason;
      try {
         // Start crash reporter after config has been successfully loaded so we have a temp directory for the dump_path
         // Start before the logger in case the logger fails
         InitializeCrashReporting();
      } catch (std::exception const& e) {
         // No crash reporting, but application can continue to run
         error_reason = std::string(e.what());
      } catch (...) {
         error_reason = "Unknown reason";
      }

      radiant::logger::init(config.GetTmpDirectory() / (config.GetName() + ".log"));

      // Have to wait for the logger to initialize before logging error
      if (!error_reason.empty()) {
         LOG(WARNING) << "Crash reporter failed to start: " + error_reason;
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
#ifdef _DEBUG
   } catch (std::exception &e) {
      LOG(WARNING) << "unhandled exception: " << e.what();
      ASSERT(false);
      //TODO: put a stop session when we implement graceful in-game quit, too.
      //TODO: Eventually pass data as to why the session stopped. Crash reporter integration?
      analytics::StopSession();
   }
#endif // _DEBUG
   return 0;
}
