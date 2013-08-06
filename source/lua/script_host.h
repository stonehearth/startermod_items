#ifndef _RADIANT_LUA_SCRIPT_HOST_H
#define _RADIANT_LUA_SCRIPT_HOST_H

#include "platform/utils.h"
#include "namespace.h"

class JSONNode;

BEGIN_RADIANT_LUA_NAMESPACE

class ScriptHost {
public:
   ScriptHost();
   ~ScriptHost();

   lua_State* GetInterpreter();
   lua_State* GetCallbackState();
   luabind::object LuaRequire(std::string name);
   void GC(platform::timer &timer);

   luabind::object JsonToLua(JSONNode const& json);

   template <typename T, typename A0, typename A1, typename A2, typename A3>
   T CallFunction(A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3) {
      try {
         luabind::object caller(cb_thread_, a0);
         return luabind::call_function<T>(caller, a1, a2, a3);
      } catch (std::exception& e) {
         OnError(e.what());
         throw;
      }
   }

   template <typename T, typename A0, typename A1, typename A2>
   T CallFunction(A0 const& a0, A1 const& a1, A2 const& a2) {
      return CallFunction<T>(a0, a1, a2, luabind::object());
   }
   template <typename T, typename A0, typename A1>
   T CallFunction(A0 const& a0, A1 const& a1) {
      return CallFunction<T>(a0, a1, luabind::object());
   }
   template <typename T, typename A0>
   T CallFunction(A0 const& a0) {
      return CallFunction<T>(a0, luabind::object());
   }

private:
   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);
   luabind::object LoadScript(std::string path);
   void Log(std::string str);

public:
   void NotifyError(std::string const& error, std::string const& traceback);
   
private:
   void OnError(std::string description);
   void AssertFailed(std::string reason);

private:
   lua_State*           L_;
   lua_State*           cb_thread_;
   std::string          lastError_;
   std::string          lastTraceback_;
   std::map<std::string, luabind::object> required_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SCRIPT_HOST_H