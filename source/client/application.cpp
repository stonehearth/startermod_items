#include "pch.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "client.h" 
#include "application.h"
#include "resources/res_manager.h"
#include "lib/perfmon/perfmon.h"
#include "lib/analytics/analytics.h"
#include "core/config.h"
#include "core/thread.h"
#include "core/process.h"
#include "poco/UUIDGenerator.h"

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

   return config.Load("stonehearth", argc, argv);
}

std::string Application::GeneratePipeName() {
   std::string uuid_string = Poco::UUIDGenerator::defaultGenerator().create().toString();
   std::string const pipe_name = "\\\\.\\pipe\\crash_reporter\\" + uuid_string;
   return pipe_name;
}

void Application::StartCrashReporter()
{
   std::string const file_name = "crash_reporter.exe";
   std::string command_line = file_name + " " + crash_dump_pipe_name_ + " " + crash_dump_path_ + " " + crash_dump_uri_;

   radiant::core::Process crash_reporter_process(command_line);
}

void Application::InitializeExceptionHandlingEnvironment()
{
   std::wstring empty_wstring;
   std::wstring pipe_name_wstring;

   pipe_name_wstring.assign(crash_dump_pipe_name_.begin(), crash_dump_pipe_name_.end());

   // This API is inconsistent, but trying to avoid too many changes to Breakpad's sample code in case it versions
   exception_handler_.reset(new ExceptionHandler(empty_wstring,                 // local dump path (ignored since we are out of process)
                                                 nullptr,                       // filter callback 
                                                 nullptr,                       // minidump callback
                                                 nullptr,                       // context for the callbacks
                                                 ExceptionHandler::HANDLER_ALL, // exceptions to trap
                                                 MiniDumpNormal,                // type of minidump
                                                 pipe_name_wstring.data(),      // name of pipe for out of process dump
                                                 nullptr));                     // CustomClientInfo

   // Do not show dialog for invalid parameter failures
   // Let the crash reporter handle it
   _CrtSetReportMode(_CRT_ASSERT, 0);
}

void Application::InitializeCrashReporting()
{
   crash_dump_path_ = core::Config::GetInstance().GetTmpDirectory().string();
   crash_dump_pipe_name_ = GeneratePipeName();
   crash_dump_uri_ = "http://posttestserver.com/post.php";

   try {
      StartCrashReporter();
      InitializeExceptionHandlingEnvironment();
   } catch (...) {
      // no crash reporting, but application can continue to run
   }
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

      // do this after config has been successfully loaded so we have a temp directory for the dump_path
      InitializeCrashReporting();

      radiant::logger::init(config.GetTmpDirectory() / (config.GetName() + ".log"));

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

      radiant::core::Thread client_thread(ClientThreadMain);

      game_engine::arbiter::GetInstance().Run();
      client_thread.Join();

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
