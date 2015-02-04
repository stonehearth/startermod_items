#include <sstream>
#include "radiant.h"
#include "process.h"
#include "namespace.h"
#include "radiant_macros.h"
#include "radiant_exceptions.h"

using namespace radiant::core;

Process::Process(std::string const& command_line) :
   detached_(false)
{
   Start(nullptr, command_line.c_str());
}

Process::Process(std::string const& binary, std::string const& command_line) :
   detached_(false)
{
   Start(binary.c_str(), command_line.c_str());
}

void Process::Start(const char* binary, const char* cmdline)
{
#ifdef WIN32
   STARTUPINFO si = { 0 };
   si.cb = sizeof si;

   memset(&process_information_, 0, sizeof process_information_);

   char mutable_cmdline[MAX_PATH];
   strncpy(mutable_cmdline, cmdline, MAX_PATH - 1);
   mutable_cmdline[MAX_PATH - 1] = '\0';

   int result = CreateProcess(binary,                   // application name
                              mutable_cmdline,          // command line
                              nullptr,                  // default process security
                              nullptr,                  // default thread security
                              FALSE,                    // inherit handles
                              0,                        // normal priority
                              nullptr,                  // inherit parent's environment variables
                              nullptr,                  // same working directory as parent
                              &si,                      // startup info
                              &process_information_);   // returned process info
   if (!result) {
      unsigned int error_code = GetLastError();
      LOG(app, 1) << "radiant::core::Process.StartProcess failed to create process. Error code:" << error_code;
   }
#else
#error class radiant::core:Process::Process() not implemented
#endif
}

Process::~Process()
{
#ifdef WIN32

   if (!detached_) {
      TerminateProcess(process_information_.hProcess, 1);
   }

   CloseHandle(process_information_.hThread);
   CloseHandle(process_information_.hProcess);
   memset(&process_information_, 0, sizeof process_information_);

#else
#error class radiant::core:Process::~Process() not implemented
#endif
}

bool Process::IsRunning()
{
   if (!process_information_.hThread) {
      return false;
   }
   DWORD result = WaitForSingleObject(process_information_.hThread, 0);
   if (result == WAIT_OBJECT_0) {
      return false;
   }
   return true;
}

void Process::Detach()
{
   detached_ = true;
}

void Process::Join()
{
#ifdef WIN32
   unsigned int result = WaitForSingleObject(process_information_.hProcess, INFINITE);
   if (result == WAIT_FAILED) {
      unsigned int error_code = GetLastError();
      LOG_CRITICAL() << "radiant::core::Process::Join failed with error code " << error_code;
   }
#else
#error radiant::core:Process::Join not implemented
#endif
}
