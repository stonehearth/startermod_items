#include "pch.h"
#include "core/config.h"
#include "client/application.h"
#include "chromium/app/app.h"
#include <luabind/luabind.hpp>

// #include "GFx.h"

using namespace ::radiant;

int lua_main(int argc, const char** argv)
{
   CefRefPtr<CefApp> app = new chromium::App;
   CefMainArgs main_args(GetModuleHandle(NULL));

   // The chromium embedded process is the current executable launched with certain parameters (defined by chrome)
   // CefExecuteProcess returns true to the parent process
   // CefExecuteProcess does not return in the child process
   if (!CefExecuteProcess(main_args, app)) {
      throw std::exception("Chromium embedded failed to start");
   }

   // Child process never gets here
   return radiant::client::Application().Run(argc, argv);
}
