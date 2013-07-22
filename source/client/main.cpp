#include "pch.h"
#include "core/config.h"
#include "client/application.h"
#include "chromium/app/app.h"
#include <luabind/luabind.hpp>

// #include "GFx.h"

using namespace ::radiant;

void protobuf_log_handler(google::protobuf::LogLevel level, const char* filename,
                          int line, const std::string& message)
{
   DLOG(INFO) << message;
}

int lua_main(lua_State* L, int argc, const char** argv)
{
   {
      CefRefPtr<CefApp> app = new chromium::App;
      CefMainArgs main_args(GetModuleHandle(NULL));
      if (!CefExecuteProcess(main_args, app)) {
         return -1;
      }
   }

   radiant::logger::init();
   // factor this out into some protobuf helper like we did with log
   google::protobuf::SetLogHandler(protobuf_log_handler);
   
   radiant::client::Application app;
   int retval = app.run(L, argc, argv);

   radiant::logger::exit();
   return retval;
}
