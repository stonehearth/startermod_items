#ifndef _RADIANT_LUA_SCRIPT_HOST_H
#define _RADIANT_LUA_SCRIPT_HOST_H

#include "platform/utils.h"
#include "lib/lua/bind.h"

class JSONNode;

BEGIN_RADIANT_LUA_NAMESPACE

class ScriptHost {
public:
   ScriptHost();
   ~ScriptHost();

   lua_State* GetInterpreter();
   lua_State* GetCallbackThread();

   luabind::object Require(std::string const& name);
   luabind::object RequireScript(std::string const& path);
   void GC(platform::timer &timer);

   typedef std::function<luabind::object(lua_State* L, JSONNode const& json)> JsonToLuaFn;
   void AddJsonToLuaConverter(JsonToLuaFn fn);

   luabind::object JsonToLua(JSONNode const& json);
   JSONNode LuaToJson(luabind::object obj);

   template <typename T, typename A0, typename A1, typename A2, typename A3, typename A4>
   T CallFunction(A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4) {
      try {
         luabind::object caller(cb_thread_, a0);
         return luabind::call_function<T>(caller, a1, a2, a3, a4);
      } catch (std::exception& e) {
         OnError(e.what());
         throw;
      }
   }

   template <typename T, typename A0, typename A1, typename A2, typename A3>
   T CallFunction(A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3) {
      return CallFunction<T>(a0, a1, a2, a3, luabind::object());
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

public: // the static interface
   static ScriptHost* GetScriptHost(lua_State*);
   static lua_State* GetCallbackThread(lua_State* L) { return GetScriptHost(L)->GetCallbackThread(); }
   static luabind::object Require(lua_State* L, std::string const& path) { return GetScriptHost(L)->Require(path); }
   static luabind::object RequireScript(lua_State* L, std::string const& path) { return GetScriptHost(L)->RequireScript(path); }
   static luabind::object JsonToLua(lua_State* L, JSONNode const& json) { return GetScriptHost(L)->JsonToLua(json); }
   static JSONNode LuaToJson(lua_State* L, luabind::object obj) { return GetScriptHost(L)->LuaToJson(obj); }

private:
   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);
   void Log(std::string str);

public:
   void NotifyError(std::string const& error, std::string const& traceback);
   
private:
   luabind::object LoadScript(std::string path);
   void OnError(std::string description);
   void AssertFailed(std::string reason);
   luabind::object GetManifest(std::string const& mod_name);
   luabind::object GetJson(std::string const& mod_name);

private:
   lua_State*           L_;
   lua_State*           cb_thread_;
   std::string          lastError_;
   std::string          lastTraceback_;
   std::map<std::string, luabind::object> required_;
   std::vector<JsonToLuaFn>   to_lua_converters_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SCRIPT_HOST_H