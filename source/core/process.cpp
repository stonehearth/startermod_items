#include <sstream>
#include "radiant.h"
#include "process.h"
#include "namespace.h"
#include "radiant_macros.h"
#include "radiant_exceptions.h"

using namespace radiant::core;

Process::Process(std::string const& command_line)
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
#error class radiant::core:Process not implemented
#endif
}
