#ifndef _RADIANT_PLATFORM_WORKER_THREAD_H
#define _RADIANT_PLATFORM_WORKER_THREAD_H

#include "thread.h"
#include "namespace.h"

BEGIN_RADIANT_PLATFORM_NAMESPACE

class WorkerThread : public Thread {
   public:
      WorkerThread();

   protected:
      void Run(void *param) override;
};

END_RADIANT_PLATFORM_NAMESPACE

#endif // _RADIANT_PLATFORM_WORKER_THREAD_H
