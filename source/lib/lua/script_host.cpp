#include "pch.h"
#include <stdexcept>
#include "lauxlib.h"
#include "radiant_file.h"
#include "core/config.h"
#include "core/system.h"
#include "core/static_string.h"
#include "core/profiler.h"
#include "script_host.h"
#include "client/renderer/render_entity.h"
#include "lib/lua/lua.h"
#include "lib/json/namespace.h"
#include "lib/perfmon/timer.h"
#include "lib/perfmon/store.h"
#include "lib/perfmon/report.h"
#include "lib/perfmon/timeline.h"
#include "lib/perfmon/sampling_profiler.h"
#include "om/components/data_store.ridl.h"
#include "om/components/mod_list.ridl.h"
#include "om/stonehearth.h"
#include "caching_allocator.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

namespace fs = ::boost::filesystem;

DEFINE_INVALID_JSON_CONVERSION(ScriptHost);

#define SH_LOG(level)    LOG(script_host, level)

extern "C" lua_State * lj_state_newstate(lua_Alloc f, void *ud);

const char* C_MODULE = "=[C]";

static std::string GetLuaTraceback(lua_State* L)
{
   lua_getfield(L, LUA_GLOBALSINDEX, "debug");
   if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      LUA_LOG(0) << "aborting traceback.  expected table for debug.";
      return "";
   }
   lua_getfield(L, -1, "traceback");
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 2);
      LUA_LOG(0) << "aborting traceback.  expected function for traceback.";
      return "";
   }

   LUA_LOG(1) << "generating traceback...";
   lua_call(L, 0, 1);  /* call debug.traceback */
   std::string traceback = std::string(lua_tostring(L, -1));
   lua_pop(L, 1);

   return traceback;
}

static int PCallCallbackFn(lua_State* L)
{
   if (!lua_isstring(L, 1)) { /* 'message' not a string? */
      LUA_LOG(0) << "error string missing at top of stack in PCallCallbackFn";
      return 0;  /* keep it intact */
   }
   std::string error = lua_tostring(L, 1);
   std::string traceback = GetLuaTraceback(L);
   ScriptHost::ReportLuaStackException(L, error, traceback);
   return 1;
}

static int PanicCallbackFn(lua_State *L)
{
   LUA_LOG(0) << "lua panic.  forcing application exit";
   PCallCallbackFn(L);
   exit(2);
   return 0;
}


ScriptHost* ScriptHost::GetScriptHost(lua_State *L)
{
   ScriptHost* sh = object_cast<ScriptHost*>(globals(L)["_host"]);
   if (!sh) {
      throw std::logic_error("could not find script host in interpreter");
   }
   return sh;
}

ScriptHost* ScriptHost::GetScriptHost(dm::ObjectPtr obj)
{
   return obj ? GetScriptHost(*obj) : nullptr;
}

ScriptHost* ScriptHost::GetScriptHost(dm::Object const& obj)
{
   return GetScriptHost(obj.GetStore());
}

ScriptHost* ScriptHost::GetScriptHost(dm::Store const& store)
{
   return GetScriptHost(store.GetInterpreter());
}

void ScriptHost::AddJsonToLuaConverter(JsonToLuaFn const& fn)
{
   to_lua_converters_.emplace_back(fn);
}

JSONNode ScriptHost::LuaToJson(luabind::object obj)
{
   try {
      return LuaToJsonImpl(obj);
   } catch (std::exception const& e) {
      json::Node result;
      result.set("error", e.what());
      return result;
   }
}

JSONNode ScriptHost::LuaToJsonImpl(luabind::object current_obj)
{
   std::vector<luabind::object> visited;

   std::function<JSONNode(luabind::object const&)> luaToJson;

   luaToJson = [&luaToJson, &visited, this] (luabind::object const& current_obj) -> JSONNode {
      luabind::object obj = GetJsonRepresentation(current_obj);
      int t = luabind::type(obj);
      if (t == LUA_TTABLE) {
         for (luabind::object const& o : visited) {
            if (current_obj == o) {
               throw std::logic_error("Cannot convert lua object with graph structure to JSON");
            }
         }
         visited.push_back(current_obj);
      }

      if (obj != current_obj && t == LUA_TSTRING) {
         // don't modify it.. it's already been converted
         std::string str = object_cast<std::string>(obj);
         // return JSONNode("", str);
         boost::algorithm::trim(str);
         if (str[0] == '{' || str[0] == '[') {
            return libjson::parse(str);
         }
         return JSONNode("", str);
      } else if (t == LUA_TTABLE) {

         if (IsNumericTable(obj)) {
            JSONNode result(JSON_ARRAY);
            luabind::object entry;
            int i = 1;
            for (;;) {
               luabind::object entry = obj[i++];
               if (luabind::type(entry) == LUA_TNIL) {
                  break;
               }
               result.push_back(luaToJson(entry));
            }
            return result;
         } else {
            JSONNode result(JSON_NODE);
            for (iterator i(obj), end; i != end; ++i) {
               JSONNode key = luaToJson(i.key());
               JSONNode value = luaToJson(*i);
               value.set_name(key.as_string());
               result.push_back(value);
            }
            return result;
         }
      } else if (t == LUA_TUSERDATA) {
         class_info ci = object_cast<class_info>((globals(L_)["class_info"])(obj));
         LUA_LOG(1) << "lua userdata object of type " << ci.name << " does not implement __tojson";
         return JSONNode("", "\"userdata\"");
      } else if (t == LUA_TSTRING) {
         std::string str = object_cast<std::string>(obj);
         //boost::algorithm::replace_all(str, "\"", "\\'");
         return JSONNode("", str);
      } else if (t == LUA_TNUMBER) {
         std::ostringstream formatter;
         double double_value = object_cast<double>(obj);
         int int_value = static_cast<int>(double_value);
         if (csg::IsZero(double_value - int_value)) {
            return JSONNode("", int_value);
         }
         return JSONNode("", double_value);
      } else if (t == LUA_TBOOLEAN) {
         return JSONNode("", object_cast<bool>(obj));
      } else if (t == LUA_TNIL) {
         JSONNode n("", 0);
         n.nullify();
         return n;
      } else if (t == LUA_TFUNCTION) {
         return JSONNode("", "function");
      }
      return JSONNode("", BUILD_STRING("\"" << "invalid lua type " << t << "\""));
   };
   return luaToJson(current_obj);
   throw std::logic_error("invalid lua type found while converting to json");
}

luabind::object ScriptHost::JsonToLua(JSONNode const& json)
{
   using namespace luabind;

   for (const auto& fn : to_lua_converters_) {
      object o = fn(cb_thread_, json);
      if (o.is_valid()) {
         return o;
      }
   }

   if (json.type() == JSON_NODE) {
      object table = newtable(L_);
      for (auto const& entry : json) {
         table[entry.name()] = JsonToLua(entry);
      }
      return table;
   } else if (json.type() == JSON_ARRAY) {
      object table = newtable(L_);
      for (unsigned int i = 0; i < json.size(); i++) {
         table[i + 1] = JsonToLua(json[i]);
      }
      return table;
   } else if (json.type() == JSON_STRING) {
      return object(L_, json.as_string());
   } else if (json.type() == JSON_NUMBER) {
      return object(L_, json.as_float());
   } else if (json.type() == JSON_BOOL) {
      return object(L_, json.as_bool());
   } else if (json.type() == JSON_NULL) {
      return object();
   }

   return object();
}

luabind::object ScriptHost::GetJson(std::string const& uri)
{
   luabind::object result;
   res::ResourceManager2::GetInstance().LookupJson(uri, [&](const json::Node& json) {
      result = JsonToLua(json.get_internal_node());
   });
   return result;
}

res::AnimationPtr ScriptHost_LoadAnimation(std::string const& uri)
{
   return res::ResourceManager2::GetInstance().LookupAnimation(uri);
}

luabind::object ScriptHost::GetConfig(std::string const& flag)
{
   core::Config & config = core::Config::GetInstance();
   if (config.Has(flag)) {
      JSONNode node = config.Get<JSONNode>(flag);
      return JsonToLua(node);
   }
   return luabind::object();
}

IMPLEMENT_TRIVIAL_TOSTRING(ScriptHost);

void ScriptHost::ProfileHookFn(lua_State *L, lua_Debug *ar)
{
   ScriptHost* sh = ScriptHost::GetScriptHost(L);
   if (sh) {
      sh->ProfileHook(L, ar);
   }
}

void ScriptHost::ProfileHook(lua_State *L, lua_Debug *ar)
{
   perfmon::CounterValueType now = perfmon::Timer::GetCurrentCounterValueType();
   if (_lastHookL == L) {
      lua_Debug f;
      int count = 0;
      res::ResourceManager2& rm = res::ResourceManager2::GetInstance();
      perfmon::CounterValueType delta = now - _lastHookTimestamp;
      
      _profilerDuration = _profilerDuration + delta;
      _profilerSampleCounts++;

      perfmon::StackFrame* current = _profilers[L].GetTopInvertedStackFrame();
      while (lua_getstack(L, count++, &f)) {
         lua_getinfo(L, "Sl", &f);
         if (strcmp(f.source, C_MODULE)) {
            current = current->AddStackFrame(f.source, f.linedefined);
            current->IncrementCount(delta, f.currentline);
         }
      }
   }
   _lastHookL = L;
   _lastHookTimestamp = now;
}

int RegisterThreadFn(lua_State* L)
{
   ScriptHost* s = ScriptHost::GetScriptHost(L);
   if (s) {
      lua_State* thread = lua_tothread(L, 1);
      s->RegisterThread(thread);
   }
   return 0;
}

void ScriptHost::RegisterThread(lua_State* L)
{
   ASSERT(!shut_down_);

   allThreads_.insert(L);
   lua_atpanic(L, PanicCallbackFn);
   if (_cpuProfilerRunning) {
      InstallProfileHook(L);
   }
}

ScriptHost::ScriptHost(std::string const& site) :
   site_(site),
   error_count(0),
   cb_thread_(nullptr),
   L_(nullptr),
   shut_down_(false),
   _lastHookL(nullptr),
   enable_profile_cpu_(false),
   _cpuProfilerRunning(false),
   _lastHookTimestamp(0)
{
   current_line = 0;
   *current_file = '\0';
   current_file[ARRAY_SIZE(current_file) - 1] = '\0';

   bytes_allocated_ = 0;

   throw_on_lua_exceptions_ = core::Config::GetInstance().Get<bool>("lua.throw_on_lua_exceptions", false);
   filter_c_exceptions_ = core::Config::GetInstance().Get<bool>("lua.filter_exceptions", true);
   enable_profile_memory_ = core::Config::GetInstance().Get<bool>("lua.enable_memory_profiler", false);
   enable_profile_cpu_ = core::Config::GetInstance().Get<bool>("lua.enable_cpu_profiler", false);
   if (enable_profile_cpu_) {
      _cpuProfileInstructionSamplingRate = core::Config::GetInstance().Get<int>("lua.profiler_instruction_sampling_rate", 15000);
   }
   std::string gc_setting = core::Config::GetInstance().Get<std::string>("lua.gc_setting", "auto");

   if (gc_setting == "auto") {
      _gc_setting = 0;
   } else if (gc_setting == "step") {
      _gc_setting = 1;
   } else {
      // "full"
      _gc_setting = 2;
   }

   // Allocate the interpreter.  64-bit builds of LuaJit require using luaL_newstate.
   bool is64Bit = core::System::IsProcess64Bit();
   bool isJitEnabled = lua::JitIsEnabled();

   // Written as the cross product of is64Bit and isJitEnabled for clarity
   if (is64Bit) {
      if (isJitEnabled) {
         // 64-bit builds of LuaJit use their own internal allocator which takes memory.
         // Use luaL_newstate to create the interpreter.  Use luaL_newstate to allocate
         // the interpreter and don't call any of the setalloc functions         
         L_ = lj_state_newstate(CachingAllocator::LuaAllocFn, this);
      } else {
         // 64-bit without jit.  We can use all our crazy allocators.
         L_ = lua_newstate(CachingAllocator::LuaAllocFn, this);
      }
   } else {
      if (!isJitEnabled) {
         // 32-bit without jit.  We can use all our crazy allocators.
         L_ = lua_newstate(CachingAllocator::LuaAllocFn, this);
      } else {
         // 32-bit with jit.  Don't install our LuaAllocFnWithState callbacks, as LuaJit
         // doesn't know how to call it.
         L_ = lua_newstate(CachingAllocator::LuaAllocFn, this);
      }
   }
   ASSERT(L_);
   if (enable_profile_memory_) {
      // This will fail if the jit is on.
      lua_setalloc2f(L_, LuaAllocFnWithState, this);
   }

   set_pcall_callback(PCallCallbackFn);
   luaL_openlibs(L_);

   luabind::open(L_);
   luabind::bind_class_info(L_);

   module(L_) [
      namespace_("_radiant") [
         def("set_profiler_enabled",   &core::SetProfilerEnabled),
         def("is_profiler_enabled",    &core::IsProfilerEnabled),
         def("is_profiler_available",  &core::IsProfilerAvailable),
         namespace_("core") [
            lua::RegisterType<core::StaticString>("StaticString")               
         ],
         namespace_("lua") [
            lua::RegisterType_NoTypeInfo<ScriptHost>("ScriptHost")
               .def("log",             &ScriptHost::Log)
               .def("exit",            &ScriptHost::Exit)
               .def("get_realtime",    &ScriptHost::GetRealTime)
               .def("get_log_level",   &ScriptHost::GetLogLevel)
               .def("get_config",      &ScriptHost::GetConfig)
               .def("get_mod_list",    &ScriptHost::GetModuleList)
               .def("read_object",     &ScriptHost::ReadObject)
               .def("write_object",    &ScriptHost::WriteObject)
               .def("enum_objects",    &ScriptHost::EnumObjects)
               .def("set_performance_counter", &ScriptHost::SetPerformanceCounter)
               .def("report_error",    (void (ScriptHost::*)(std::string const& error, std::string const& traceback))&ScriptHost::ReportLuaStackException)
               .def("require",         &ScriptHost::Require)
               .def("require_script",  &ScriptHost::RequireScript)
               .def("get_error_count", &ScriptHost::GetErrorCount)
         ]
      ]
   ];
   object radiant = globals(L_)["_radiant"];
   radiant["register_thread"] = GetPointerToCFunction(L_, &RegisterThreadFn);

   globals(L_)["_host"] = object(L_, this);

   if (!core::Config::GetInstance().Get<bool>("lua.enable_luajit", true)) {
      if (luaL_dostring(L_, "jit.off()") != 0) {
         LUA_LOG(0) << "Failed to disable jit. " << lua_tostring(L_, -1);
      } else {
         LUA_LOG(0) << "luajit disabled."; 
      }
   }

   globals(L_)["package"]["path"] = "";
   globals(L_)["package"]["cpath"] = "";

   // xxx : all c -> lua functions (except maybe update) should be on this clean callback thread.
   // this is to prevent state corruption and all sorts of other confusion which can result
   // from running on whatever state the main thread was in (e.g. if it most recently yielded
   // from a coroutine...)
   cb_thread_ = lua_newthread(L_);

   RegisterThread(L_);
   RegisterThread(cb_thread_);
}

ScriptHost::~ScriptHost()
{
   LUA_LOG(1) << "Shutting down script host.";
   FullGC();
   required_.clear();
   lua_close(L_);
   ASSERT(this->bytes_allocated_ == 0);
   LUA_LOG(1) << "Script host destroyed.";
}

void paranoid_hook(lua_State *L, lua_Debug *ar)
{
   std::string error = BUILD_STRING("LUA code executing on shutdown:\n");
   std::string traceback = GetLuaTraceback(L);

   LUA_LOG(0) << error;

   std::string item;
   std::stringstream sstb(traceback);
   while(std::getline(sstb, item)) {
      LUA_LOG(0) << "   " << item;
   }
   ASSERT(false);
}

// We can install a line-hook on shutdown, to ensure that if any Lua code is executed, we immediately assert.
// In A Perfect World, Lua should never run during a shutdown.
void ScriptHost::Shutdown()
{
   shut_down_ = true;
   lua_sethook(L_, paranoid_hook, LUA_MASKLINE, 0);
}

bool ScriptHost::IsShutDown() const
{
   return shut_down_;
}

int ScriptHost::GetErrorCount() const
{
   return error_count;
}

std::string ExtractAllocKey(lua_State *l) {
    // At this point, we know we have a valid stack to extract info from--so do that!
   lua_Debug stack;

   lua_getstack(l, 0, &stack);
   lua_getinfo(l, "nSl", &stack);

   std::string fnName(stack.name ? stack.name : "");
   std::string srcName(stack.source ? stack.source : "");

   if (srcName == "@radiant/modules/events.lua" && fnName == "listen") {
      int oldLine = stack.currentline;
      // Reach back to find the listener.
      lua_getstack(l, 1, &stack);
      lua_getinfo(l, "nSl", &stack);

      if (stack.name) {
         fnName = stack.name;
      } else {
         fnName = "";
      }
      fnName += BUILD_STRING("[listen " << oldLine << "]");
   } else if (srcName == "=[C]") {
      int oldLine = stack.currentline;
      lua_getstack(l, 1, &stack);
      lua_getinfo(l, "nSl", &stack);

      if (stack.name) {
         fnName = stack.name;
      } else {
         fnName = "";
      }
   } else if (srcName == "@radiant/modules/log.lua" && fnName == "create_logger") {
      int oldLine = stack.currentline;
      // Reach back to find the listener.
      lua_getstack(l, 1, &stack);
      lua_getinfo(l, "nSl", &stack);

      if (stack.name) {
         fnName = stack.name;
      }
      fnName += BUILD_STRING("[logger " << oldLine << "]");
   } else if (srcName == "@radiant/modules/log.lua" && fnName == "set_prefix") {
      int oldLine = stack.currentline;
      // Reach back to find the listener.
      lua_getstack(l, 1, &stack);
      lua_getinfo(l, "nSl", &stack);

      if (stack.name) {
         fnName = stack.name;
      }
      fnName += BUILD_STRING("[set_prefix " << oldLine << "]");
   } else if (srcName == "@radiant/lib/unclasslib.lua" && fnName == "build") {
      int oldLine = stack.currentline;
      // Reach back to find the listener.
      lua_getstack(l, 1, &stack);
      lua_getinfo(l, "nSl", &stack);

      if (stack.name) {
         fnName = stack.name;
      }
      fnName += BUILD_STRING("[build " << oldLine << "]");
   }

   return BUILD_STRING(stack.source << ":" << fnName << ":" << stack.currentline);
 }


/*
 * The type of the memory-allocation function used by Lua states. The allocator function must
 * provide a functionality similar to realloc, but not exactly the same. Its arguments are ud,
 * an opaque pointer passed to lua_newstate; ptr, a pointer to the block being
 * allocated/reallocated/freed; osize, the original size of the block; nsize, the new size
 * of the block. ptr is NULL if and only if osize is zero. When nsize is zero, the allocator
 * must return NULL; if osize is not zero, it should free the block pointed to by ptr. When
 * nsize is not zero, the allocator returns NULL if and only if it cannot fill the request.
 * When nsize is not zero and osize is zero, the allocator should behave like malloc. When
 * nsize and osize are not zero, the allocator behaves like realloc. Lua assumes that the
 * allocator never fails when osize >= nsize.
 */

void* ScriptHost::LuaAllocFnWithState(void *ud, void *ptr, size_t osize, size_t nsize, lua_State *L)
{
   ScriptHost* host = static_cast<ScriptHost*>(ud);
   ASSERT(host->enable_profile_memory_);

   void *realloced = CachingAllocator::LuaAllocFn(ud, ptr, osize, nsize);
   if (realloced && ptr && host->alloc_backmap[ptr] != "") {
      ASSERT(nsize > 0);
      std::string oldKey = host->alloc_backmap[ptr];
      host->alloc_map[oldKey].erase(ptr);
      host->alloc_backmap.erase(ptr);
      host->alloc_map[oldKey][realloced] = nsize;
      host->alloc_backmap[realloced] = oldKey;
   } else {
      host->alloc_map[host->alloc_backmap[ptr]].erase(ptr);
      if (host->alloc_map[host->alloc_backmap[ptr]].size() == 0) {
         host->alloc_map.erase(host->alloc_backmap[ptr]);
      }
      host->alloc_backmap.erase(ptr);
      if (realloced) {
         ASSERT(nsize > 0);
         std::string key = "unknown";
         lua_State *l = L;
         if (l != nullptr) {
            lua_Debug stack;
            int r = lua_getstack(l, 0, &stack);
            if (!r) {
               l = host->L_;

               if (l) {
                  r = lua_getstack(l, 0, &stack);
               }
            }
            if (r) {
               key = ExtractAllocKey(l);
            }
         }
         host->alloc_map[key][realloced] = nsize;
         host->alloc_backmap[realloced] = key;
      }
   }
   return realloced;
}

void ScriptHost::LuaTrackLine(lua_State *L, lua_Debug *ar)
{
   ScriptHost* s = GetScriptHost(L);
   lua_getinfo(L, "Sl", ar);
   strncpy(s->current_file, ar->source, ARRAY_SIZE(s->current_file) - 1);
   s->current_line = ar->currentline;
}

void ScriptHost::ReportCStackThreadException(lua_State* L, std::exception const& e) const
{
   std::string error = BUILD_STRING("c++ exception: " << e.what());
   std::string traceback = GetLuaTraceback(L);
   ReportStackException("native", error, traceback);
   if (!filter_c_exceptions_) {
      throw;
   }
}

void ScriptHost::ReportLuaStackException(std::string const& error, std::string const& traceback)
{
   error_count++;
   ReportStackException("lua", error, traceback);
   if (throw_on_lua_exceptions_) {
      throw;
   }
}

void ScriptHost::ReportStackException(std::string const& category, std::string const& error, std::string const& traceback) const
{
   LUA_LOG(0) << "-- Script Error (" << category << ") Begin ------------------------------- ";
   if (!error.empty()) {
      std::string item;
      std::stringstream sse(error);
      while(std::getline(sse, item)) {
         LUA_LOG(0) << "   " << item;
      }

      std::stringstream sstb(traceback);
      while(std::getline(sstb, item)) {
         LUA_LOG(0) << "   " << item;
      }
   }
   LUA_LOG(0) << "-- Lua Error End   ------------------------------- ";

   if (error_cb_) {
      om::ErrorBrowser::Record record;
      record.SetSummary(error);
      record.SetCategory(record.SEVERE);
      record.SetBacktrace(traceback);
      error_cb_(record);
   }
}

lua_State* ScriptHost::GetInterpreter()
{
   return L_;
}

lua_State* ScriptHost::GetCallbackThread()
{
   return cb_thread_;
}

void ScriptHost::FullGC()
{
   lua_gc(L_, LUA_GCCOLLECT, 0);
}

void ScriptHost::GC(platform::timer &timer)
{
   if (_gc_setting == 0) {
      return;
   } else if (_gc_setting == 1) {
      bool finished = false;

      while (!timer.expired() && !finished) {
         finished = (lua_gc(L_, LUA_GCSTEP, 1) != 0);
      }
   } else {
      FullGC();
   }
}

luabind::object ScriptHost::Require(std::string const& s)
{
   std::string path;

   std::vector<std::string> parts;
   boost::split(parts, s, boost::is_any_of("."));
   parts.erase(std::remove(parts.begin(), parts.end(), ""), parts.end());

   path = boost::algorithm::join(parts, "/") + ".lua";

   return RequireScript(path);
}

luabind::object ScriptHost::RequireScript(std::string const& path)
{
   res::ResourceManager2& rm = res::ResourceManager2::GetInstance();
   std::string canonical_path;
   try {
      canonical_path = rm.FindScript(path);
   } catch (std::exception const&) {
      // Use of exceptions here is idiotic. One of my worst ideas in a decade...
      LUA_LOG(1) << "could not find path for lua script \"" << path << "\".";
      return luabind::object();
   }

   luabind::object obj;

   auto i = required_.find(canonical_path);
   if (i != required_.end()) {
      obj = i->second;
   } else {
      LUA_LOG(5) << "requiring script " << canonical_path;
      required_[canonical_path] = luabind::object();
      obj = rm.LoadScript(L_, canonical_path);
      required_[canonical_path] = obj;
   }
   return obj;
}

void ScriptHost::Log(const char* category, int level, const char* str)
{
   if (category && str) {
      LOG_CATEGORY_(level, BUILD_STRING("mod " << category)) << str;
   }
}

void ScriptHost::Exit(int code)
{
   LOG(core.system, 0) << "exiting with code " << code << " by mod request.";
   // NOOOOOOOOOOOO!  Can we gracefully shutdown?  So far this is used exclusively
   // by the autotest framework.
   TerminateProcess(GetCurrentProcess(), code);
}


double ScriptHost::GetRealTime()
{
   return perfmon::Timer::GetCurrentTimeMs() / 1000.0;
}

int ScriptHost::GetLogLevel(std::string const& category)
{
   static const int SENTINEL = 0xd3adb33f;
   size_t last = category.size();

   // Walk backwards until we can find a can find a value for this thing
   // that matches. (e.g. try logging.mods.stonehearth.events.log_level, then
   // logging.mods.stonehearth.log_level, then logging.mods.log_level...
   int config_level = SENTINEL;
   while (last != std::string::npos) {
      std::string path = category.substr(0, last);
      std::string flag = BUILD_STRING("logging.mods." << path);
      config_level = log::GetLogLevel(flag, SENTINEL);
      if (config_level != SENTINEL) {
         break;
      }
      last = category.rfind('.', last - 1);
   }
   if (config_level == SENTINEL) {
      config_level = log::GetDefaultLogLevel();
   }
   return config_level;
}

bool ScriptHost::CoerseToBool(object const& o)
{
   bool result = false;
   if (o) {
      switch (type(o)) {
      case LUA_TNIL:
         return false;
      case LUA_TBOOLEAN:
         return object_cast<bool>(o);
      default:
         return true;
      }
   }
   return result;
}

void ScriptHost::SetNotifyErrorCb(ReportErrorCb const& cb)
{
   error_cb_ = cb;
}

int ScriptHost::GetAllocBytesCount() const
{
   return bytes_allocated_;
}

void ScriptHost::Trigger(std::string const& eventName, luabind::object evt)
{
   try {
      luabind::object radiant = globals(cb_thread_)["radiant"];
      TriggerOn(radiant, eventName, evt);
   } catch (std::exception const& e) {
      ReportCStackThreadException(cb_thread_, e);
   }
}

void ScriptHost::TriggerOn(luabind::object obj, std::string const& eventName, luabind::object evt)
{
   try {
      if (!evt || !evt.is_valid()) {
         evt = luabind::newtable(L_);
      }
      luabind::object radiant = globals(cb_thread_)["radiant"];
      radiant["events"]["trigger"](obj, eventName, evt);
   } catch (std::exception const& e) {
      ReportCStackThreadException(cb_thread_, e);
   }
}

void ScriptHost::DumpHeap(std::string const& filename) const
{
   std::unordered_map<std::string, int> keyMap;
   FILE* outf = fopen(filename.c_str(), "wb");
   
   // Dump keys first.  Create an index for the keys, too.
   int numkeys = (int)alloc_map.size();
   fwrite(&numkeys, sizeof(numkeys), 1, outf);
   int keyidx = 0;
   for (const auto& v : alloc_map) {
      keyMap[v.first] = keyidx;

      // + 1 for the null terminator!
      fwrite(v.first.c_str(), 1, v.first.size() + 1, outf);

      keyidx++;
   }

   // Record and write the total number of memory allocations.
   int totalallocs = 0;
   for (const auto& entry : alloc_map) {
      totalallocs += (int)entry.second.size();
   }
   fwrite(&totalallocs, sizeof(totalallocs), 1, outf);
   for (const auto& entry : alloc_map) {
      int idx = keyMap[entry.first];
      for (const auto& alloc : entry.second) {
         // key (redundancy makes things easier)
         fwrite(&idx, sizeof(idx), 1, outf);

         // ptr
         fwrite(&alloc.first, sizeof(idx), 1, outf);

         // size
         fwrite(&alloc.second, sizeof(idx), 1, outf);

         // write the bytes!
         fwrite(alloc.first, 1, alloc.second, outf);
      }
   }

   fclose(outf);
}

typedef struct {
   std::string key;
   int allocs;
   int totalBytes;
} _MemSample;

void ScriptHost::WriteMemoryProfile(std::string const& filename)
{
   if (!enable_profile_memory_) {
      return;
   }

   FullGC();

   std::vector<_MemSample> samples;
   int grand_total = 0;
   unsigned int w = 0;
   for (const auto& entry : alloc_map) {
      int total = 0;
      for (const auto& alloc : entry.second) {
         total += (int)alloc.second;
      }
      grand_total += total;
      _MemSample s;
      s.key = entry.first;
      s.allocs = (int)entry.second.size();
      s.totalBytes = total;
      samples.push_back(s);
      w = std::max(w, (unsigned int)entry.first.size() + 2);
   }

   std::ofstream f(filename);
   auto format = [](int size) {
      static std::string suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB" /* lol! */ };
      std::string* suffix = suffixes;
      float total = static_cast<float>(size);
      while (total > 1024.0) {
         total /= 1024.0;
         suffix++;
      }
      return BUILD_STRING(std::fixed << std::setw(5) << std::setprecision(3) << total << " " << *suffix);
   };

   auto output = [&f, &format, w](std::string const& txt, int c, int size) {
      f << std::left << std::setw(w) << txt << " : " << format(size);
      if (c) {
        f << " (" << c << ", " << format(size / c) << " ea)";
      }
      f << std::endl;
   };

   std::sort(samples.begin(), samples.end(), [](_MemSample const& a, _MemSample const& b) -> bool {
      return a.totalBytes > b.totalBytes;
   });

   output("Total Memory Tracked", 0, grand_total);
   output("Total Memory Allocated", 0, GetAllocBytesCount());
   for (const auto& sample : samples) {
      output(sample.key, sample.allocs, sample.totalBytes);
   }
   LUA_LOG(0) << " wrote lua memory profile data to lua_memory_profile.txt";
}

void ScriptHost::ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter) const
{
   addCounter("lua:alloced_bytes", GetAllocBytesCount(), "memory");
   for (auto const& entry : performanceCounters_) {
      addCounter(entry.first.c_str(), entry.second.first, entry.second.second.c_str());
   }
}

void ScriptHost::SetPerformanceCounter(const char* name, double value, const char* kind)
{
   performanceCounters_[BUILD_STRING("lua:" << name)] = std::make_pair(value, kind);
}

luabind::object ScriptHost::GetJsonRepresentation(luabind::object obj) const
{
   int type = luabind::type(obj);
   if (type == LUA_TTABLE) {
      // If we have __saved_variable, route over there first.  This happens routinely
      // when saving controllers.  They have a __saved_variables member which points
      // to their DataStore.
      luabind::object __saved_variables = obj["__saved_variables"];
      int svType = luabind::type(__saved_variables);
      if (svType != LUA_TNIL) {
         type = svType;
         obj = __saved_variables;
      }
   }

   if (type == LUA_TTABLE || type == LUA_TUSERDATA) {
      try {
         // If there's a __tojson function, call it.
         luabind::object __tojson = obj["__tojson"];
         if (__tojson && __tojson.is_valid()) {
            luabind::object json = __tojson(obj);
            if (json && json.is_valid()) {
               obj = json;
            }
         }
      } catch (std::exception const& e) {
         LUA_LOG(1) << "call to __to_json failed: " << e.what();
         ReportCStackThreadException(L_, e);
      }
   }

   return obj;
}
 

void ScriptHost::AddObjectToLuaConvertor(dm::ObjectType type, ObjectToLuaFn const& cast_fn)
{
   ASSERT(!object_cast_table_[type]);
   object_cast_table_[type] = cast_fn;
}

luabind::object ScriptHost::CastObjectToLua(dm::ObjectPtr obj)
{
   if (!obj) {
      return luabind::object();
   }

   ObjectToLuaFn cast_fn = object_cast_table_[obj->GetObjectType()];
   ASSERT(cast_fn);
   return cast_fn(L_, obj);
}

bool ScriptHost::IsNumericTable(luabind::object tbl) const
{
   iterator i(tbl), end;

   // used to treat empty tables as numeric
   object n = tbl["n"];
   if (n.is_valid() && type(n) == LUA_TNUMBER) {
      return true;
   }
   if (i == end) {
      return false; // assume empty tables are objects.
   }
   return luabind::type(tbl) == LUA_TTABLE && luabind::type(tbl[1]) != LUA_TNIL;
}

void ScriptHost::LoadGame(om::ModListPtr mods, AllocDataStoreFn allocd, std::unordered_map<dm::ObjectId, om::EntityPtr>& em, std::vector<om::DataStorePtr>& datastores)
{
   // Two passes: First create all the controllers for the datastores we just
   // created.

   SH_LOG(7) << "restoring datastores controllers";
   for (om::DataStorePtr datastore : datastores) {
      datastore->RestoreController(datastore);
   }
   SH_LOG(7) << "finished restoring datastores controllers";

   // Now run through all the tables on those datastores and convert the
   // pointers-to-datastore to pointers-to-controllers
   SH_LOG(7) << "restoring datastores controller data";
   for (om::DataStorePtr datastore : datastores) {
      datastore->RestoreControllerData();
   }
   SH_LOG(7) << "finished restoring datastores controller data";

   CreateModules(mods, allocd);

   for (om::DataStorePtr datastore : datastores) {
      try {
         luabind::object controller = datastore->GetController();
         if (controller.is_valid()) {
            object restore_fn = controller["restore"];
            if (type(restore_fn) == LUA_TFUNCTION) {
               restore_fn(controller);
            }
            object activate_fn = controller["activate"];
            if (type(activate_fn) == LUA_TFUNCTION) {
               activate_fn(controller);
            }
         }
      } catch (std::exception const& e) {
         ReportCStackException(L_, e);
      }
   }

   SH_LOG(7) << "restoring lua components";
   for (auto const& entry : em) {
      om::EntityPtr entity = entry.second;
      om::Stonehearth::RestoreLuaComponents(this, entity);
   }
   SH_LOG(7) << "finished restoring lua components";

   Trigger("radiant:game_loaded");
}

void ScriptHost::CreateGame(om::ModListPtr mods, AllocDataStoreFn allocd)
{
   CreateModules(mods, allocd);

   core::Config const& config = core::Config::GetInstance();
   std::string const module = config.Get<std::string>("game.main_mod", "stonehearth");
   luabind::object obj = mods->GetMod(module);
   if (obj) {
      TriggerOn(obj, "radiant:new_game", luabind::newtable(L_));
   }
}

void ScriptHost::CreateModules(om::ModListPtr mods, AllocDataStoreFn allocd)
{
   res::ResourceManager2 &resource_manager = res::ResourceManager2::GetInstance();

   CreateModule(mods, "radiant", allocd);
   for (std::string const& mod_name : resource_manager.GetModuleNames()) {
      if (mod_name != "radiant") {
         CreateModule(mods, mod_name, allocd);
      }
   }
   Require("radiant.lib.strict");
   Trigger("radiant:required_loaded");
}

luabind::object ScriptHost::GetModuleList() const
{
   int i = 1;
   luabind::object result = luabind::newtable(L_);
   JSONNode modules = res::ResourceManager2::GetInstance().GetModules();
   for (auto const& entry : modules) {
      result[i++] = luabind::object(L_, entry.name());
   }
   return result;
}

bool ScriptHost::WriteObject(const char* modname, const char* objectName, luabind::object o)
{
   fs::path path = core::System::GetInstance().GetTempDirectory() / "saved_objects" / modname / (std::string(objectName) + ".json");

   try {
      fs::path parent = path.parent_path();
      if (!fs::is_directory(parent)) {
         fs::create_directories(parent);
      }
      std::ofstream os(path.string());
      os << LuaToJson(o).write_formatted();
      os.close();
   } catch (std::exception const& e) {
      LUA_LOG(1) << "failed to create " << path << ": " << e.what();
      return false;
   }
   return true;
}


luabind::object ScriptHost::ReadObject(const char* modname, const char* objectName)
{
   luabind::object obj;

   fs::path path = core::System::GetInstance().GetTempDirectory() / "saved_objects" / modname / (std::string(objectName) + ".json");

   try {
      if (fs::is_regular_file(path)) {
         std::ifstream is(path.string());
         std::string jsonfile = io::read_contents(is);
         is.close();

         obj = JsonToLua(libjson::parse(jsonfile));
      }
   } catch (std::exception const& e) {
      LUA_LOG(1) << "failed to read " << path.string() << ": " << e.what();
   }
   return obj;
}

luabind::object ScriptHost::EnumObjects(const char* modname, const char* path)
{
   luabind::object objects = luabind::newtable(L_);

   fs::path modpath = core::System::GetInstance().GetTempDirectory() / "saved_objects" / modname / std::string(path);

   try {
      if (fs::is_directory(modpath)) {
         int start = (int)modpath.string().size() + 1;

         int count = 1;
         fs::directory_iterator const end;
         for (fs::directory_iterator i(modpath); i != end; i++) {
            fs::path const path = i->path();
            if (fs::is_regular_file(path) && path.filename().extension() == ".json") {
               std::string pathstr = path.string();
               std::string name = pathstr.substr(start, pathstr.size() - start - 5 );
               objects[count++] = name;
            }
         }
      }
   } catch (std::exception const& e) {
      LUA_LOG(1) << "failed to enum objects at " << modpath.string() << ": " << e.what();
   }

   return objects;
}


luabind::object ScriptHost::CreateModule(om::ModListPtr mods, std::string const& mod_name, AllocDataStoreFn allocDs)
{
   res::ResourceManager2 &resource_manager = res::ResourceManager2::GetInstance();
   std::string script_name;
   luabind::object module;

   std::string scriptKey = BUILD_STRING(site_ << "_init_script");
   resource_manager.LookupManifest(mod_name, [&](const res::Manifest& manifest) {
      script_name = manifest.get<std::string>(scriptKey, "");
   });
   if (!script_name.empty()) {
      try {
         luabind::object savestate = mods->GetMod(mod_name);

         if (!savestate || !savestate.is_valid()) {
            om::DataStoreRef datastore = allocDs(mods->GetStore().GetStoreId());
            datastore.lock()->SetData(luabind::newtable(L_));
            savestate = luabind::object(L_, datastore);
         }
         module = Require(script_name);
         if (!module || luabind::type(module) != LUA_TTABLE) { 
            LUA_LOG(1) << "module \"" << mod_name << "\" returned non-table type.  ignoring";
            return luabind::object();
         }
         module["__saved_variables"] = savestate;
         luabind::globals(L_)[mod_name] = module;
         mods->AddMod(mod_name, module);
         TriggerOn(module, "radiant:init", luabind::object());
      } catch (std::exception const& e) {
         LUA_LOG(1) << "module " << mod_name << " failed to load " << script_name << ": " << e.what();
      }
   }
   return module;
}

bool ScriptHost::ToggleCpuProfiling()
{
   if (!enable_profile_cpu_) {
      SH_LOG(1) << "cpu profiler not enabled.  set lua.enable_cpu_profiler to true";
      return false;
   }

   _cpuProfilerRunning = !_cpuProfilerRunning;
   for (lua_State* L : allThreads_) {
      if (_cpuProfilerRunning) {
         InstallProfileHook(L);
      } else {
         RemoveProfileHook(L);
      }
   }

   if (!_cpuProfilerRunning) {
      int bottomUpDepth = 2;

      perfmon::FunctionTimes stats;
      perfmon::FunctionAtLineTimes bottomUpStats;
      res::ResourceManager2& rm = res::ResourceManager2::GetInstance();

      for (auto& entry : _profilers) {
         perfmon::SamplingProfiler &sp = entry.second;
         sp.FinalizeCollection(rm);
         sp.CollectStats(stats);
         sp.CollectBottomUpStats(bottomUpStats, bottomUpDepth);
      }
      _profilers.clear();

      int msPerSample = 0;
      if (_profilerSampleCounts) {
         msPerSample  = perfmon::CounterToMilliseconds(_profilerDuration / _profilerSampleCounts);
      }
      LOG(lua.code, 1) << "---- lua cpu profilers stats --------------------------------------";
      LOG(lua.code, 1) << "   profiler_instruction_sampling_rate: " << _cpuProfileInstructionSamplingRate << " instructions";
      LOG(lua.code, 1) << "   profiling duration:                 " << perfmon::CounterToMilliseconds(_profilerDuration) << " ms";
      LOG(lua.code, 1) << "   samples taken:                      " << _profilerSampleCounts;
      LOG(lua.code, 1) << "   average ms per sample interval:     " << msPerSample << " ms";
      LOG(lua.code, 1) << "---- total lua time -----------------------------------------------";
      perfmon::ReportCounters<perfmon::FunctionTimes, 30>(stats, [](core::StaticString const& name, perfmon::CounterValueType const& duration, size_t rows) {
         LOG(lua.code, 1) << std::setw(120) << name << " : "
                           << std::setw(8) << perfmon::CounterToMilliseconds(duration) << " ms : "
                           << std::string(rows, '#');
      });

      LOG(lua.code, 1) << "---- bottom up time (" << bottomUpDepth << " deep) -----------------------------------------------";
      perfmon::ReportCounters<perfmon::FunctionAtLineTimes, 30>(bottomUpStats, [](std::string const& name, perfmon::CounterValueType const& duration, size_t rows) {
         LOG(lua.code, 1) << std::setw(120) << name << " : "
                           << std::setw(8) << perfmon::CounterToMilliseconds(duration) << " ms : "
                           << std::string(rows, '#');
      });
   }

   _profilerDuration = 0;
   _profilerSampleCounts = 0;

   return _cpuProfilerRunning;
}

void ScriptHost::InstallProfileHook(lua_State* L)
{
   lua_sethook(L, ScriptHost::ProfileHookFn, LUA_MASKCOUNT, _cpuProfileInstructionSamplingRate);
}

void ScriptHost::RemoveProfileHook(lua_State* L)
{
   lua_sethook(L, nullptr, 0, 0);
}
