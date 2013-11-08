#ifndef _RADIANT_CORE_PROCESS_H
#define _RADIANT_CORE_PROCESS_H

#include <string>
#include "namespace.h"
#include "radiant_macros.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Process
{
public:
   // Creates a new process with the supplied command line
   Process(std::string const& command_line);

   // Terminates the process
   // Call Detach to allow the process to continue beyond the scope of the Process object
   ~Process();

   // Separates the executing process from the Process object, allowing execution to continue independently
   void Detach();

   // Wait for Process to finish executing
   void Join();

private:
   NO_COPY_CONSTRUCTOR(Process);

   bool detached_;

#ifdef WIN32
   PROCESS_INFORMATION process_information_;
#endif
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_PROCESS_H
