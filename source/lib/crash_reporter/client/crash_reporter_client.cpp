#include "crash_reporter_client.h"
#include "radiant_macros.h"
#include "core/process.h"
#include "build_number.h"
#include "poco/UUIDGenerator.h"
#include <sstream>
#include <mutex>

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

std::string CrashReporterClient::GeneratePipeName() const {
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

void CrashReporterClient::Start(std::string const& crash_dump_path, std::string const& crash_dump_uri, std::string const& userid)
{
   ASSERT(!running_);

   std::string const pipe_name = GeneratePipeName();
   std::string const filename = CRASH_REPORTER_NAME + ".exe";
   std::string const command_line = BUILD_STRING(filename << " " << pipe_name << " " << crash_dump_path << " " << crash_dump_uri << " " << userid);

   crash_reporter_server_process_.reset(new radiant::core::Process(command_line));

   InitializeExceptionHandlingEnvironment(pipe_name);
   
   running_ = true;
}

// This static method must be thread safe!
void CrashReporterClient::RunWithExceptionWrapper(std::function<void()> const& fn, bool const terminate_on_error)
{
   if (running_) {
      // run naked and let crash reporter catch exceptions
      // currently don't have a way to report the exception message to the user / modder when breakpad is active
      // catch and rethrow might disrupt the stack that breakpad needs to read
      fn();
   } else {
      try {
         fn();
      } catch (std::exception const& e) {
         std::string const error_message = BUILD_STRING("Error: " << e.what());
         LOG(ERROR) << error_message;

         // currently unused
         if (terminate_on_error) {
            TerminateApplicationWithMessage(error_message);
         }
      }
   }
}

// This static method must be thread safe!
void CrashReporterClient::TerminateApplicationWithMessage(std::string const& error_message)
{
   static std::mutex mutex;

   {
      std::lock_guard<std::mutex> lock(mutex);
      MessageBox(nullptr, error_message.c_str(), PRODUCT_IDENTIFIER, MB_OK);
   }
   std::terminate();
}
