#include "crash_reporter_client.h"
#include "radiant_macros.h"
#include "core/process.h"
#include "poco/UUIDGenerator.h"
#include <sstream>

 // google_breakpad
#include "client/windows/handler/exception_handler.h"

// Uncomment if assertions should be handled by Breakpad in DEBUG builds
//#define ENABLE_BREAKPAD_IN_DEBUG_BUILDS

using namespace radiant;
using namespace radiant::crash_reporter::client;

static std::string const NAMED_PIPE_PREFIX = "\\\\.\\pipe\\";
static std::string const CRASH_REPORTER_NAME = "crash_reporter";

DEFINE_SINGLETON(CrashReporterClient);

bool CrashReporterClient::running_ = false;

#ifdef DEBUG
bool const CrashReporterClient::debug_build_ = true;
#else
bool const CrashReporterClient::debug_build_ = false;
#endif

std::string CrashReporterClient::GeneratePipeName() {
   std::string const uuid_string = Poco::UUIDGenerator::defaultGenerator().create().toString();
   std::string const pipe_name = BUILD_STRING(NAMED_PIPE_PREFIX << CRASH_REPORTER_NAME << "\\" << uuid_string);
   return pipe_name;
}

void CrashReporterClient::InitializeExceptionHandlingEnvironment(std::string const& crash_dump_pipe_name)
{
#ifdef ENABLE_BREAKPAD_IN_DEBUG_BUILDS
   _CrtSetReportMode(_CRT_ASSERT, 0);
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

bool CrashReporterClient::Start(std::string const& crash_dump_path, std::string const& crash_dump_uri, std::string& error_string)
{
   ASSERT(!running_);

   try {
      std::string const pipe_name = GeneratePipeName();
      std::string const filename = CRASH_REPORTER_NAME + ".exe";
      std::string const command_line = BUILD_STRING(filename << " " << pipe_name << " " << crash_dump_path << " " << crash_dump_uri);

      crash_reporter_server_process_.reset(new radiant::core::Process(command_line));

      InitializeExceptionHandlingEnvironment(pipe_name);
   
      // no exception was thrown, so we're up
      running_ = true;
      error_string.clear();
   } catch (std::exception const& e) {
      // eat the exception and let the app run without a crash reporter
      error_string = e.what();
   } catch (...) {
      error_string = "Unknown reason";
   }

   return running_;
}

// This static method must be thread safe!
void CrashReporterClient::RunWithExceptionWrapper(std::function<void()> fn)
{
   if (running_ && !debug_build_) {
      // run naked and let crash reporter catch exceptions
      fn();
   } else {
      std::string error_string;
      try {
         fn();
         return;
      } catch (std::exception const& e) {
         error_string = e.what();
      } catch (...) {
         error_string = "Unknown reason";
      }
      // Make the catch behavior a function if caller wants to control behavior
      LOG(ERROR) << "Unhandled exception: " << error_string;
   }
}
