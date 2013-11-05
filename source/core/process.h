#ifndef _RADIANT_CORE_PROCESS_H
#define _RADIANT_CORE_PROCESS_H

#include <string>
#include "namespace.h"
#include "radiant_macros.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Process
{
public:
   Process(std::string const& command_line);

private:
   NO_COPY_CONSTRUCTOR(Process);

#ifdef WIN32
   PROCESS_INFORMATION process_information_;
#endif
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_PROCESS_H
