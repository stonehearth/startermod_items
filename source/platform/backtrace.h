#ifndef _RADIANT_PLATFORM_BACKTRACE_H
#define _RADIANT_PLATFORM_BACKTRACE_H

namespace radiant {
   namespace platform {
      LONG WINAPI PrintBacktrace(EXCEPTION_POINTERS* pExp, DWORD dwExpCode);
   };
};

#endif // _RADIANT_PLATFORM_BACKTRACE_H
