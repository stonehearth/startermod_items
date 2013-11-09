#ifndef _RADIANT_CRASH_REPORTER_SERVER_CRASH_REPORTER_SERVER_H
#define _RADIANT_CRASH_REPORTER_SERVER_CRASH_REPORTER_SERVER_H

#include "radiant.h"
#include "namespace.h"
#include "core/singleton.h"
#include <functional>

namespace google_breakpad {
   class ClientInfo;
   class CrashGenerationServer;
}

BEGIN_RADIANT_CRASH_REPORTER_SERVER_NAMESPACE

class CrashReporterServer : public radiant::core::Singleton<CrashReporterServer>
{
public:
   // Starts a crash reporter server to dump and upload the process state when the application crashes
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

   // need a shared_ptr to use a forward declaration for CrashGenerationServer
   std::shared_ptr<google_breakpad::CrashGenerationServer> crash_server_;
   std::function<void()> exit_process_function_;
};

END_RADIANT_CRASH_REPORTER_SERVER_NAMESPACE

#endif // _RADIANT_CRASH_REPORTER_SERVER_CRASH_REPORTER_SERVER_H
