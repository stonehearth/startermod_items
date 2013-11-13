#include "pch.h"
#include "core/config.h"
#include "client/application.h"
#include "chromium/app/app.h"
#include <luabind/luabind.hpp>

// #include "GFx.h"

using namespace ::radiant;


int lua_main(int argc, const char** argv)
{
#ifdef _DEBUG
   try {
#endif // _DEBUG
      {
         CefRefPtr<CefApp> app = new chromium::App;
         CefMainArgs main_args(GetModuleHandle(NULL));
         if (!CefExecuteProcess(main_args, app)) {
            return -1;
         }
      }
      return radiant::client::Application().Run(argc, argv);
#ifdef _DEBUG
   } catch (std::exception const& e) {
      LOG(WARNING) << "!!!!!!!!!!!! UNCAUGHT EXCEPTION !!!!!!!!!!!!!";
      LOG(WARNING) << e.what();
   }
#endif // _DEBUG
   return 1;
}
