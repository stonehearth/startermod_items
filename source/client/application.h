#ifndef _RADIANT_CLIENT_APPLICATION_H
#define _RADIANT_CLIENT_APPLICATION_H

#include "core/process.h"
#include "game_engine.h"
#include "client/windows/handler/exception_handler.h" // google_breakpad
//#include "configfile.h"

extern "C" struct lua_State;

namespace radiant {
   namespace client {
      class Application {
         public:
            Application();
            ~Application();
               
         public:
            int Run(int argc, const char **argv);

         private:
            bool LoadConfig(int argc, const char** argv);
            bool InitializeCrashReporting(std::string& error_string);

            static void ClientThreadMain();
      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
