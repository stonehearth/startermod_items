#include "radiant.h"
#include "platform/Thread.h"

using namespace radiant::platform;

ThreadId radiant::platform::GetCurrentThreadId()
{
   return ::GetCurrentThreadId();
}

Thread::Thread() :
   _thread(NULL),
   _thread_id(0),
   _running(false)
{
   InitializeCriticalSection(&_lock);
}

void Thread::Start(ThreadFn fn)
{
   _fn = fn;
   _thread = ::CreateThread(NULL, 0, thread_main, this, 0, &_thread_id);
}

DWORD WINAPI Thread::thread_main(LPVOID param)
{
   Thread *t = (Thread *)param;

   static int affinity = 1;
   SetThreadAffinityMask(GetCurrentThread(), 1 << affinity);
   affinity++;

   t->SetRunning(true);
   t->_fn();
   t->SetRunning(false);

   return 0;
}

bool Thread::IsRunning()
{
   bool running;
   EnterCriticalSection(&_lock);
   running = _running;
   LeaveCriticalSection(&_lock);
   return running;
}

void Thread::SetRunning(bool value)
{
   EnterCriticalSection(&_lock);
   _running = value;
   LeaveCriticalSection(&_lock);
}

