#ifndef _RADIANT_PLATFORM_THREAD_POOL_H
#define _RADIANT_PLATFORM_THREAD_POOL_H

#include "thread.h"
#include <functional>
#include <vector>
#include <list>

BEGIN_RADIANT_PLATFORM_NAMESPACE

class ThreadPool {
   public:
      typedef std::function<void ()> WorkFn;
      ThreadPool(int threadCount);

      void Stop();
      void Wait();

      void SubmitWork(WorkFn fn);

   protected:
      bool GetWork(ThreadPool::WorkFn &work);
      static void Work(ThreadPool &pool);

   protected:
      std::vector<Thread>       _threads;
      std::list<WorkFn>         _tasks;
      bool                 _running;
      CRITICAL_SECTION     _tasksLock;
      CONDITION_VARIABLE   _taskAvailableCV;
};

END_RADIANT_PLATFORM_NAMESPACE

#endif // _RADIANT_PLATFORM_THREAD_POOL_H
