#include "radiant.h"
#include "game_engine.h"
#include "resources/res_manager.h"
#include <luabind/luabind.hpp>

using namespace radiant;
using namespace luabind;

static lua_State* L_ = nullptr;

BOOL WINAPI DllMain(
  _In_  HINSTANCE hinstDLL,
  _In_  DWORD fdwReason,
  _In_  LPVOID lpvReserved
)
{
   if (fdwReason == DLL_PROCESS_ATTACH) {
      printf("attached!\n");
   }
   return true;
}

void start(lua_State* L, object args)
{
   std::cout << "starting server..." << std::endl;

   int argc = 1;
   const char* argv[64] = { "" };

   if (type(args) == LUA_TTABLE) {
      argc = lua_objlen(L, -1) + 1;
      for (int i = 0; i < argc; i++) {
         argv[i] = object_cast<const char*>(args[i]);
      }
   }

   extern int lua_main(lua_State* L, int argc, const char** argv);
   lua_main(L_, argc, argv);
}

extern "C" __declspec(dllexport) int init(lua_State* L)
{
   L_ = L;

   open(L);
   module(L) [
      def("start", &start)
   ];
   return 0;
}
