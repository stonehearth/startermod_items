#include "radiant.h"
#include "chromium/app/app.h"

using namespace radiant;
using namespace radiant::chromium;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   CefRefPtr<CefApp> app = new App;
   CefMainArgs main_args(GetModuleHandle(NULL));
   return CefExecuteProcess(main_args, app);
}
