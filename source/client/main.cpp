#include "pch.h"
#include "core/config.h"
#include "client/application.h"

// #include "GFx.h"

using namespace ::radiant;

int lua_main(int argc, const char** argv)
{
   CefRefPtr<CefApp> app = new chromium::App;
   CefMainArgs main_args(GetModuleHandle(NULL));

   // CefExecuteProcess returns -1 if we're the main app (i.e. the browser process).
   // All the sample code and documentation uses >= 0 to verify this, so that's
   // what I'm doing too.
   int exitcode = CefExecuteProcess(main_args, app, nullptr);
   if (exitcode >= 0) {
      return exitcode;
   }
   return radiant::client::Application().Run(argc, argv);
}
