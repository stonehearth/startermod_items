#ifndef _RADIANT_PLATFORM_THREAD_H
#define _RADIANT_PLATFORM_THREAD_H

#include "namespace.h"
#include <functional>

BEGIN_RADIANT_PLATFORM_NAMESPACE

typedef DWORD  ThreadId;

class Thread {
   public:
      Thread();
      typedef std::function <void()> ThreadFn;

      void Start(ThreadFn fn);
      bool IsRunning();

   protected:
      void SetRunning(bool value);

#if defined(WIN32)
      static DWORD WINAPI thread_main(LPVOID param);

      HANDLE            _thread;
      DWORD             _thread_id;
      bool              _running;
      ThreadFn          _fn;
      CRITICAL_SECTION  _lock;
#endif
};

ThreadId GetCurrentThreadId();

END_RADIANT_PLATFORM_NAMESPACE

#endif // _RADIANT_PLATFORM_THREAD_H
