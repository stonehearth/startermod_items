#ifndef _RADIANT_CLIENT_APPLICATION_H
#define _RADIANT_CLIENT_APPLICATION_H

#include "game_engine.h"
#include "client/windows/handler/exception_handler.h"
//#include "configfile.h"

extern "C" struct lua_State;

using namespace google_breakpad;

namespace radiant {
   namespace client {
      class Application {
         public:
            Application();
            ~Application();
               
         public:
            int Run(int argc, const char **argv);

         private:
            std::unique_ptr<ExceptionHandler> exception_handler_;
            std::string crash_dump_pipe_name_;
            std::string crash_dump_path_;
            std::string crash_dump_uri_;

            bool LoadConfig(int argc, const char** argv);
            std::string GeneratePipeName();
            void StartCrashReporter();
            void InitializeExceptionHandlingEnvironment();
            void InitializeCrashReporting();

            static void ClientThreadMain();
      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
