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
#ifdef WIN32
   std::string mutable_command_line(command_line);
   STARTUPINFO si = {};

   int result = CreateProcess(nullptr,                  // application name
                              &mutable_command_line[0], // command line
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
      throw core::Exception(BUILD_STRING("radiant::core::Process.StartProcess failed to create process. Error code " << error_code));
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
#else
#error class radiant::core:Process::~Process() not implemented
#endif
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
      LOG(WARNING) << "radiant::core::Thread.Join failed with error code " << error_code;
   }
#else
#error radiant::core:Process::Join not implemented
#endif
}
