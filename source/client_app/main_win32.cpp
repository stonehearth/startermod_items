#include <signal.h>
#include "radiant.h"

struct lua_State;
extern int lua_main(lua_State* L, int argc, const char **argv);

static void f()
{
   static volatile int i = 0;
   i++;
}

static void f2(int)
{
   f();
}

int filter(struct _EXCEPTION_POINTERS *ep, unsigned int code) {
   f();
   return 0;
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   std::set_terminate(f);
   signal(SIGABRT, f2);

   __try {
      atexit(f);
      return lua_main(NULL, __argc, (const char **)__argv);
   //} __except (radiant::platform::PrintBacktrace(GetExceptionInformation(), GetExceptionCode())) {
   } __except (filter(GetExceptionInformation(), GetExceptionCode())) {
      return 1;
   }
   return 0;
}
