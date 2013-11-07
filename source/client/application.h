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
            std::unique_ptr<google_breakpad::ExceptionHandler> exception_handler_;
            std::unique_ptr<radiant::core::Process> crash_reporter_process_;

            bool LoadConfig(int argc, const char** argv);
            std::string GeneratePipeName();
            void StartCrashReporter(std::string const& crash_dump_path, std::string const& crash_dump_pipe_name, std::string const& crash_dump_uri);
            void InitializeExceptionHandlingEnvironment(std::string const& crash_dump_pipe_name);
            void InitializeCrashReporting();

            static void ClientThreadMain();
      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
