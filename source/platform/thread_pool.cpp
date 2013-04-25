#include "radiant.h"
#include "thread_pool.h"
#include <algorithm>

using ::radiant::platform::ThreadPool;

ThreadPool::ThreadPool(int threadCount) :
   _threads(threadCount),
   _running(true)
{
   InitializeCriticalSection(&_tasksLock);
   InitializeConditionVariable(&_taskAvailableCV);

   std::for_each(_threads.begin(), _threads.end(), [&](Thread &worker) {
      worker.Start(std::bind(ThreadPool::Work, *this));
   });
}

void ThreadPool::Stop()
{
   EnterCriticalSection(&_tasksLock);
   _tasks.clear();
   _running = false;
   WakeAllConditionVariable(&_taskAvailableCV);
   LeaveCriticalSection(&_tasksLock);
}

void ThreadPool::Wait()
{
   Stop();
   bool finished = true;
   do {
      EnterCriticalSection (&_tasksLock);
      for (unsigned int i = 0; i < _threads.size(); i++) {
         if (_threads[i].IsRunning()) {
            finished = false;
         }
      }
      LeaveCriticalSection(&_tasksLock);
   } while (!finished);
}

bool ThreadPool::GetWork(ThreadPool::WorkFn &work)
{
   EnterCriticalSection(&_tasksLock);
   while (_tasks.empty() && _running) {
      SleepConditionVariableCS(&_taskAvailableCV, &_tasksLock, INFINITE);
   }
   if (!_running) {
      LeaveCriticalSection(&_tasksLock);
      return false;
   }
   work = _tasks.front();
   _tasks.pop_front();
   LeaveCriticalSection(&_tasksLock);
   return true;
}

void ThreadPool::SubmitWork(WorkFn fn)
{
   EnterCriticalSection (&_tasksLock);
   _tasks.push_back(fn);
   LeaveCriticalSection(&_tasksLock);
   WakeConditionVariable(&_taskAvailableCV);
}


void ThreadPool::Work(ThreadPool &pool)
{
   ThreadPool::WorkFn work;

   while (pool.GetWork(work)) {
      work();
   }
}