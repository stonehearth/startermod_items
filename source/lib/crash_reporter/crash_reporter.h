#ifndef _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H
#define _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H

#include "resource.h"
#include "radiant.h"
#include "core/singleton.h"
#include <functional>

// The google headers are ugly so forward declare the types instead
namespace google_breakpad {
   class ClientInfo;
   class CrashGenerationServer;
}

using namespace radiant;

class CrashReporter : public core::Singleton<CrashReporter>
{
public:
   void Run(std::string const& pipe_name, std::string const& dump_path, std::string const& uri,
            std::function<void()> const& exit_process_function);

private:
   bool StartCrashGenerationServer();
   void SendCrashReport(std::string const& dump_filename);
   void CreateZip(std::string const& zip_filename, std::string const& dump_filename);
   void ExitProcess();

   // Breakpad callbacks
   static void OnClientConnected(void* context, google_breakpad::ClientInfo const* client_info);
   static void OnClientCrashed(void* context, google_breakpad::ClientInfo const* client_info, std::wstring const* dump_filename_w);
   static void OnClientExited(void* context, google_breakpad::ClientInfo const* client_info);

   std::string pipe_name_;
   std::string dump_path_;
   std::string uri_;
   std::unique_ptr<google_breakpad::CrashGenerationServer> crash_server_;
   std::function<void()> exit_process_function_;
};

#endif // _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H
