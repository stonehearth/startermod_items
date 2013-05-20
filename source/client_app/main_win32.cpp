#include "radiant.h"

struct lua_State;
extern int lua_main(lua_State* L, int argc, const char **argv);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if 0
   __try {
      return main(__argc, __argv);
   } __except (radiant::platform::PrintBacktrace(GetExceptionInformation(), GetExceptionCode())) {
      return 1;
   }
   return 0;
#else
   return lua_main(NULL, __argc, (const char **)__argv);
#endif
}
