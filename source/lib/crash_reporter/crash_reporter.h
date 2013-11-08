#ifndef _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H
#define _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H

#include "resource.h"
#include "client/windows/crash_generation/crash_generation_server.h" // google_breakpad

class CrashReporter
{
public:
   CrashReporter(std::string const& pipe_name, std::string const& dump_path, std::string const& uri);
   void SendCrashReport(std::string const& dump_filename);

private:
   bool StartCrashGenerationServer();
   void CreateZip(std::string const& zip_filename, std::string const& dump_filename);

   // Breakpad callbacks
   static void OnClientConnected(void* context, google_breakpad::ClientInfo const* client_info);
   static void OnClientCrashed(void* context, google_breakpad::ClientInfo const* client_info, std::wstring const* dump_filename_w);
   static void OnClientExited(void* context, google_breakpad::ClientInfo const* client_info);
   static void RequestApplicationExit();

   std::string const pipe_name_;
   std::string const dump_path_;
   std::string const uri_;
   std::unique_ptr<google_breakpad::CrashGenerationServer> crash_server_;
};

#endif // _RADIANT_LIB_CRASH_REPORTER_CRASH_REPORTER_H
