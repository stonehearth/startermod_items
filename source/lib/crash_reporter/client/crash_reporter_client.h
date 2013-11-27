#ifndef _RADIANT_CRASH_REPORTER_CLIENT_CRASH_REPORTER_CLIENT_H
#define _RADIANT_CRASH_REPORTER_CLIENT_CRASH_REPORTER_CLIENT_H

#include "radiant.h"
#include "namespace.h"
#include "core/singleton.h"
#include "core/process.h"

namespace google_breakpad {
   class ExceptionHandler;
}

BEGIN_RADIANT_CRASH_REPORTER_CLIENT_NAMESPACE

// Important note on threading:
// Breakpad (the crash reporter) doesn't work with std::thread or poco::thread (poco::thread with dynamic linking might work).
// They do not have the proper top level exception handling behavior which is configured
// using SetUnhandledExceptionFilter on Windows.
// Windows CreateThread and boost::thread appear to work
class CrashReporterClient : public radiant::core::Singleton<CrashReporterClient>
{
public:
   // Install exception hooks and create the out-of-process server to catch crashes
   // Returns false on error with the message in error_string
   void Start(std::string const& crash_dump_path, std::string const& crash_dump_uri, std::string const& userid);

   // Thread-safe function that wraps either Breakpad or Windows exception handling around your function
   static void CrashReporterClient::RunWithExceptionWrapper(std::function<void()> const& fn, bool const terminate_on_error = false);
   static void TerminateApplicationWithMessage(std::string const& error_message);

private:
   std::string GeneratePipeName() const;
   void InitializeExceptionHandlingEnvironment(std::string const& crash_dump_pipe_name);

   std::unique_ptr<google_breakpad::ExceptionHandler> exception_handler_;
   std::unique_ptr<radiant::core::Process> crash_reporter_server_process_;

   static bool running_;
};

END_RADIANT_CRASH_REPORTER_CLIENT_NAMESPACE

#endif // _RADIANT_CRASH_REPORTER_CLIENT_CRASH_REPORTER_CLIENT_H
