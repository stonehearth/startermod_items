#ifndef _RADIANT_CLIENT_APPLICATION_H
#define _RADIANT_CLIENT_APPLICATION_H

#include "game_engine.h"
#include "lua/script_host.h"

//#include "configfile.h"

extern "C" struct lua_State;

namespace radiant {
   namespace client {
      class Application {
         public:
            Application();
            ~Application();
               
         public:
            int run(lua_State* L, int argc, const char **argv);

         private:
            bool LoadConfig(int argc, const char** argv);
            int Start(lua_State* L);
//            configlib::configfile   _config;
         private:
            lua::ScriptHost   scriptHost_;
      };
   };
};

#endif // _RADIANT_CLIENT_APPLICATION_H
