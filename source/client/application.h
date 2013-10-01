#ifndef _RADIANT_CLIENT_APPLICATION_H
#define _RADIANT_CLIENT_APPLICATION_H

#include "game_engine.h"

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

      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
