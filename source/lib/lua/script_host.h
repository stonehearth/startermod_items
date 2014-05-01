#ifndef _RADIANT_LUA_SCRIPT_HOST_H
#define _RADIANT_LUA_SCRIPT_HOST_H

#include "platform/utils.h"
#include "lib/lua/bind.h"
#include "om/error_browser/error_browser.h"
#include "om/om.h"
#include "lib/perfmon/perfmon.h"

class JSONNode;

BEGIN_RADIANT_LUA_NAMESPACE

class ScriptHost {
public:
   ScriptHost(std::string const& site);
   ~ScriptHost();

   lua_State* GetInterpreter();
   lua_State* GetCallbackThread();

   void CreateGame(om::ModListPtr mods);
   void LoadGame(om::ModListPtr mods, std::unordered_map<dm::ObjectId, om::EntityPtr>& em);

   luabind::object Require(std::string const& name);
   luabind::object RequireScript(std::string const& path);
   void GC(platform::timer &timer);
   void FullGC();
   int GetAllocBytesCount() const;
   void ClearMemoryProfile();
   void ProfileMemory(bool value);
   void WriteMemoryProfile(std::string const& filename) const;
   void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter) const;
   int GetErrorCount() const;

   typedef std::function<luabind::object(lua_State* L, JSONNode const& json)> JsonToLuaFn;
   void AddJsonToLuaConverter(JsonToLuaFn fn);

   luabind::object JsonToLua(JSONNode const& json);
   JSONNode LuaToJson(luabind::object obj);
     
   void ReportCStackThreadException(lua_State* L, std::exception const& e) const;
   void ReportLuaStackException(std::string const& error, std::string const& traceback);
   void Trigger(const std::string& eventName, luabind::object evt = luabind::object());
   void TriggerOn(luabind::object obj, const std::string& eventName, luabind::object evt = luabind::object());

   typedef std::function<void(::radiant::om::ErrorBrowser::Record const&)> ReportErrorCb;
   void SetNotifyErrorCb(ReportErrorCb const& cb);

   typedef std::function<luabind::object(lua_State*L, dm::ObjectPtr)> ObjectToLuaFn;
   void AddObjectToLuaConvertor(dm::ObjectType type,  ObjectToLuaFn cast_fn);
   luabind::object CastObjectToLua(dm::ObjectPtr obj);

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
   static ScriptHost* GetScriptHost(dm::ObjectPtr obj);
   static ScriptHost* GetScriptHost(dm::Object const& obj);
   static ScriptHost* GetScriptHost(dm::Store const& store);
   static lua_State* GetInterpreter(lua_State* L) { return GetScriptHost(L)->GetInterpreter(); }
   static lua_State* GetCallbackThread(lua_State* L) { return GetScriptHost(L)->GetCallbackThread(); }
   static luabind::object Require(lua_State* L, std::string const& path) { return GetScriptHost(L)->Require(path); }
   static luabind::object RequireScript(lua_State* L, std::string const& path) { return GetScriptHost(L)->RequireScript(path); }
   static luabind::object JsonToLua(lua_State* L, JSONNode const& json) { return GetScriptHost(L)->JsonToLua(json); }
   static JSONNode LuaToJson(lua_State* L, luabind::object obj) { return GetScriptHost(L)->LuaToJson(obj); }
   static void ReportCStackException(lua_State* L, std::exception const& e) { return GetScriptHost(L)->ReportCStackThreadException(L, e); }
   static void ReportLuaStackException(lua_State* L, std::string const& error, std::string const& traceback) { return GetScriptHost(L)->ReportLuaStackException(error, traceback); }
   static bool CoerseToBool(luabind::object const& o);

private:
   luabind::object ScriptHost::GetConfig(std::string const& flag);
   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);
   static void LuaTrackLine(lua_State *L, lua_Debug *ar);
   void Log(const char* category, int level, const char* str);
   void Exit(int code);
   int GetLogLevel(std::string const& category);
   double GetRealTime();
   void ReportStackException(std::string const& category, std::string const& error, std::string const& traceback) const;
   luabind::object GetObjectRepresentation(luabind::object o, std::string const& format) const;
   bool IsNumericTable(luabind::object tbl) const;
   void CreateModules(om::ModListPtr mods);
   luabind::object CreateModule(om::ModListPtr mods, std::string const& mod_name);

private:
   luabind::object LoadScript(const std::string& path);
   void OnError(const std::string& description);
   luabind::object GetManifest(std::string const& mod_name);
   luabind::object GetJson(std::string const& mod_name);
   void SetPerformanceCounter(const char* name, double value, const char* kind);
   
private:
   lua_State*           L_;
   lua_State*           cb_thread_;
   std::string          site_;
   std::map<std::string, luabind::object> required_;
   std::vector<JsonToLuaFn>   to_lua_converters_;
   bool                 filter_c_exceptions_;
   bool                 enable_profile_memory_;
   bool                 profile_memory_;
   ReportErrorCb        error_cb_;
   int                  bytes_allocated_;

   char                 current_file[256];
   int                  current_line;

   int                  error_count;  // Count of script errors encountered.

   typedef std::unordered_map<void*, int>          Allocations;
   std::unordered_map<void *, std::string>         alloc_backmap;
   std::unordered_map<std::string, Allocations>    alloc_map;
   std::unordered_map<std::string, std::pair<double, std::string>>   performanceCounters_;

   std::unordered_map<dm::ObjectType, ObjectToLuaFn>  object_cast_table_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SCRIPT_HOST_H