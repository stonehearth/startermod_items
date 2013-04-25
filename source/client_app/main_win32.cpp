#include "radiant.h"

struct lua_State;
extern int lua_main(lua_State* L, int argc, const char **argv);

int main(int argc, char** argv)
{
#if 0
   __try {
      return main(__argc, __argv);
   } __except (radiant::platform::PrintBacktrace(GetExceptionInformation(), GetExceptionCode())) {
      return 1;
   }
   return 0;
#else
   return lua_main(NULL, argc, (const char **)argv);
#endif
}
