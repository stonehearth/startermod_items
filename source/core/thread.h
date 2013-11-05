#ifndef _RADIANT_CORE_THREAD_H
#define _RADIANT_CORE_THREAD_H

#include "namespace.h"
#include "radiant_macros.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Thread
{
public:
   // support up to one argument for now
   Thread(void* fn, void* arg1 = nullptr);
   void Join();

private:
   NO_COPY_CONSTRUCTOR(Thread);

#ifdef WIN32
   HANDLE thread_handle_;
#endif
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_THREAD_H
