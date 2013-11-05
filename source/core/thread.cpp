#include <sstream>
#include "radiant.h"
#include "thread.h"
#include "radiant_logger.h"
#include "radiant_exceptions.h"
#include "radiant_macros.h"

using namespace radiant::core;

// support up to one argument for now
Thread::Thread(void* fn, void* arg1)
{
#ifdef WIN32
   thread_handle_ = CreateThread(nullptr,                      // default security
                                 0,                            // default stack size
                                 (LPTHREAD_START_ROUTINE)fn,   // start address
                                 arg1,                         // parameters
                                 0,                            // run immediately
                                 nullptr);                     // thread id

   if (!thread_handle_) {
      unsigned int error_code = GetLastError();
      throw core::Exception(BUILD_STRING("radiant::core::Thread failed to create thread. Error code " << error_code));
   }
#else
#error class radiant::core:Thread not implemented
#endif
}

void Thread::Join()
{
#ifdef WIN32
   unsigned int result = WaitForSingleObject(thread_handle_, INFINITE);
   if (result == WAIT_FAILED) {
      unsigned int error_code = GetLastError();
      LOG(WARNING) << "radiant::core::Thread.Join failed with error code " << error_code;
   }
#else
#error radiant::core:Thread.Join not implemented
#endif
}
