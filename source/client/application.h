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
            std::string pipe_name_;
            std::string dump_path_;

            bool LoadConfig(int argc, const char** argv);
            std::string GuidToString(GUID const& guid);
            std::string GeneratePipeName();
            void StartCrashReporter();
            void InitializeExceptionHandlingEnvironment();
            void InitializeCrashReporting();

            static void ClientThreadMain();
      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
