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
   typedef std::function<om::DataStoreRef(int storeId)> AllocDataStoreFn;
public:
   ScriptHost(std::string const& site, AllocDataStoreFn const& allocDs);
   ~ScriptHost();

   lua_State* GetInterpreter();
   lua_State* GetCallbackThread();

   void CreateGame(om::ModListPtr mods);
   void LoadGame(om::ModListPtr mods, std::unordered_map<dm::ObjectId, om::EntityPtr>& em, std::vector<om::DataStorePtr>& datastores);
   void Shutdown();
   bool IsShutDown() const;

   luabind::object Require(std::string const& name);
   luabind::object RequireScript(std::string const& path);

   void GC(platform::timer &timer);
   void FullGC();
   int GetAllocBytesCount() const;
   void WriteMemoryProfile(std::string const& filename);
   void DumpHeap(std::string const& filename) const;
   void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter) const;
   int GetErrorCount() const;

   typedef std::function<luabind::object(lua_State* L, JSONNode const& json)> JsonToLuaFn;
   void AddJsonToLuaConverter(JsonToLuaFn const& fn);

   luabind::object JsonToLua(JSONNode const& json);
   JSONNode LuaToJson(luabind::object obj);
     
   void ReportCStackThreadException(lua_State* L, std::exception const& e) const;
   void ReportLuaStackException(std::string const& error, std::string const& traceback);
   void Trigger(std::string const& eventName, luabind::object evt = luabind::object());
   void TriggerOn(luabind::object obj, std::string const& eventName, luabind::object evt = luabind::object());

   typedef std::function<void(::radiant::om::ErrorBrowser::Record const&)> ReportErrorCb;
   void SetNotifyErrorCb(ReportErrorCb const& cb);

   typedef std::function<luabind::object(lua_State*L, dm::ObjectPtr)> ObjectToLuaFn;
   void AddObjectToLuaConvertor(dm::ObjectType type,  ObjectToLuaFn const& cast_fn);
   luabind::object CastObjectToLua(dm::ObjectPtr obj);

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
   static void* LuaAllocFnWithState(void *ud, void *ptr, size_t osize, size_t nsize, lua_State* L);
   static void LuaTrackLine(lua_State *L, lua_Debug *ar);
   void Log(const char* category, int level, const char* str);
   void Exit(int code);
   int GetLogLevel(std::string const& category);
   double GetRealTime();
   void ReportStackException(std::string const& category, std::string const& error, std::string const& traceback) const;
   luabind::object GetJsonRepresentation(luabind::object o) const;
   bool IsNumericTable(luabind::object tbl) const;
   void CreateModules(om::ModListPtr mods);
   luabind::object CreateModule(om::ModListPtr mods, std::string const& mod_name);
   luabind::object GetModuleList() const;
   JSONNode LuaToJsonImpl(luabind::object obj);

private:
   luabind::object LoadScript(std::string const& path);
   luabind::object GetManifest(std::string const& mod_name);
   luabind::object GetJson(std::string const& mod_name);
   void SetPerformanceCounter(const char* name, double value, const char* kind);
   bool WriteObject(const char* modname, const char* objectName, luabind::object o);
   luabind::object ReadObject(const char* modname, const char* objectName);
   luabind::object EnumObjects(const char* modname, const char* path);

private:
   lua_State*           L_;
   lua_State*           cb_thread_;
   std::string          site_;
   std::map<std::string, luabind::object> required_;
   std::vector<JsonToLuaFn>   to_lua_converters_;
   bool                 filter_c_exceptions_;
   bool                 throw_on_lua_exceptions_;
   ReportErrorCb        error_cb_;
   int                  bytes_allocated_;

   char                 current_file[256];
   int                  current_line;

   int                  error_count;  // Count of script errors encountered.

   // Memory profiling
   bool                 enable_profile_memory_;
   int                  _gc_setting;
   typedef std::unordered_map<void*, size_t>       Allocations;
   std::unordered_map<void *, std::string>         alloc_backmap;
   std::unordered_map<std::string, Allocations>    alloc_map;

   // CPU profiling
   bool                 enable_profile_cpu_;
   bool                 profile_cpu_;

   std::unordered_map<std::string, std::pair<double, std::string>>   performanceCounters_;

   std::unordered_map<dm::ObjectType, ObjectToLuaFn>  object_cast_table_;

   AllocDataStoreFn     _allocDs;

   bool                 shut_down_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_SCRIPT_HOST_H