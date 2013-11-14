#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <fcntl.h>

#include "radiant.h"
#include "log_internal.h"

using namespace radiant::logger;

#if defined(_DEBUG)
#pragma warning(push)
#pragma warning(disable:4996)
static void redirect_io_to_console()
{
   CONSOLE_SCREEN_BUFFER_INFO coninfo;
   CONSOLE_FONT_INFOEX fontinfo;

   ::AllocConsole();

   ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
   coninfo.dwSize.X = 140;
   coninfo.dwSize.Y = 900;
   ::SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

   ::GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &fontinfo);
   wcscpy(fontinfo.FaceName, L"Lucidia Console");
   ::SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &fontinfo);

   freopen("conin$","r", stdin);
   freopen("conout$","w",stdout);
   freopen("conout$","w",stderr);
}
#pragma warning(pop)
#endif

void radiant::logger::platform_init()
{
   DEBUG_ONLY(redirect_io_to_console();)
}

void radiant::logger::platform_exit()
{
}
