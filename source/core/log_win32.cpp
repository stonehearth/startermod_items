#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <fcntl.h>

#include "radiant.h"
#include "core/config.h"

using namespace radiant::logger;

#pragma warning(push)
#pragma warning(disable:4996)
void radiant::logger::InitConsole()
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

   freopen("CONOUT$", "wb", stdout);
   freopen("CONOUT$", "wb", stderr);
}
#pragma warning(pop)
