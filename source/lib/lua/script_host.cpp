#include "pch.h"
#include <stdexcept>
#include "lauxlib.h"
#include "radiant_file.h"
#include "core/config.h"
#include "script_host.h"
#include "lua_supplemental.h"
#include "client/renderer/render_entity.h"
#include "lib/json/namespace.h"
#include "lib/perfmon/timer.h"
#include "lib/perfmon/store.h"
#include "lib/perfmon/timeline.h"
#include "om/components/data_store.ridl.h"

extern "C" int luaopen_lpeg (lua_State *L);

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

DEFINE_INVALID_JSON_CONVERSION(ScriptHost);
DEFINE_INVALID_LUA_CONVERSION(ScriptHost)

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

void ScriptHost::AddJsonToLuaConverter(JsonToLuaFn fn)
{
   to_lua_converters_.emplace_back(fn);
}

JSONNode ScriptHost::LuaToJson(luabind::object current_obj)
{
   std::vector<luabind::object> visited;

   std::function<JSONNode(luabind::object const&)> luaToJson;

   luaToJson = [&luaToJson, &visited, this] (luabind::object const& current_obj) -> JSONNode {
      luabind::object obj = GetObjectRepresentation(current_obj, "__tojson");
      int t = luabind::type(obj);
      if (t == LUA_TTABLE) {
         for (luabind::object const& o : visited) {
            if (current_obj == o) {
               return JSONNode("", "visited node");
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
            for (iterator i(obj), end; i != end; i++) {
               JSONNode key = luaToJson(i.key());
               JSONNode value = luaToJson(*i);
               value.set_name(key.as_string());
               result.push_back(value);
            }
            return result;
         }
      } else if (t == LUA_TUSERDATA) {
         class_info ci = call_function<class_info>(globals(L_)["class_info"], obj);
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
         return JSONNode("", nullptr);
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
      object o = fn(L_, json);
      if (o.is_valid()) {
         return o;
      }
   }

   if (json.type() == JSON_NODE) {
      object table = newtable(L_);
      for (auto const& entry : json) {
         table[entry.name()] = JsonToLua(L_, entry);
      }
      return table;
   } else if (json.type() == JSON_ARRAY) {
      object table = newtable(L_);
      for (unsigned int i = 0; i < json.size(); i++) {
         table[i + 1] = JsonToLua(L_, json[i]);
      }
      return table;
   } else if (json.type() == JSON_STRING) {
      return object(L_, json.as_string());
   } else if (json.type() == JSON_NUMBER) {
      return object(L_, json.as_float());
   } else if (json.type() == JSON_BOOL) {
      return object(L_, json.as_bool());
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

res::AnimationPtr ScriptHost_LoadAnimation(std::string uri)
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

ScriptHost::ScriptHost() 
{
   current_line = 0;
   *current_file = '\0';
   current_file[ARRAY_SIZE(current_file) - 1] = '\0';

   bytes_allocated_ = 0;
   filter_c_exceptions_ = core::Config::GetInstance().Get<bool>("lua.filter_exceptions", true);
   enable_profile_memory_ = core::Config::GetInstance().Get<bool>("lua.enable_memory_profiler", false);
   profile_memory_ = false;

   L_ = lua_newstate(LuaAllocFn, this);
   set_pcall_callback(PCallCallbackFn);
   luaL_openlibs(L_);
   luaopen_lpeg(L_);

   luabind::open(L_);
   luabind::bind_class_info(L_);

   module(L_) [
      namespace_("_radiant") [
         namespace_("lua") [
            lua::RegisterType<ScriptHost>("ScriptHost")
               .def("log",             &ScriptHost::Log)
               .def("exit",            &ScriptHost::Exit)
               .def("get_realtime",    &ScriptHost::GetRealTime)
               .def("get_log_level",   &ScriptHost::GetLogLevel)
               .def("get_config",      &ScriptHost::GetConfig)
               .def("set_performance_counter", &ScriptHost::SetPerformanceCounter)
               .def("report_error",    (void (ScriptHost::*)(std::string const& error, std::string const& traceback) const)&ScriptHost::ReportLuaStackException)
               .def("require",         (luabind::object (ScriptHost::*)(std::string const& name))&ScriptHost::Require)
               .def("require_script",  (luabind::object (ScriptHost::*)(std::string const& name))&ScriptHost::RequireScript)
         ]
      ]
   ];
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
}

ScriptHost::~ScriptHost()
{
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

void* ScriptHost::LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize)
{
   ScriptHost* host = static_cast<ScriptHost*>(ud);
   void *realloced;

   host->bytes_allocated_ += (nsize - osize);
   
   if (nsize == 0) {
      delete [] ptr;
      realloced = nullptr;
   } else {
      realloced = new char[nsize];
      if (osize) {
         memcpy(realloced, ptr, std::min(nsize, osize));
         delete [] ptr;
      }
   }

   if (host->profile_memory_) {
      std::string const& key = host->alloc_backmap[ptr];
      host->alloc_map[key].erase(ptr);
      host->alloc_backmap.erase(ptr);
      if (realloced) {
         std::string key = BUILD_STRING(host->current_file << ":" << host->current_line);
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

void ScriptHost::ReportLuaStackException(std::string const& error, std::string const& traceback) const
{
   ReportStackException("lua", error, traceback);
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

void ScriptHost::GC(platform::timer &timer)
{
   bool finished = false;

   while (!timer.expired() && !finished) {
      finished = (lua_gc(L_, LUA_GCSTEP, 1) != 0);
   }
}

luabind::object ScriptHost::LoadScript(std::string path)
{
   std::ifstream in;
   luabind::object obj;

   LUA_LOG(5) << "loading script " << path;
   
   int error;
   try {
      error = luaL_loadfile_from_resource(L_, path.c_str());
   } catch (std::exception const& e) {
      LUA_LOG(1) << e.what();
      return luabind::object();
   }

   if (error == LUA_ERRFILE) {
      OnError("Coud not open script file " + path);
   } else if (error != 0) {
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return obj;
   }
   if (lua_pcall(L_, 0, LUA_MULTRET, 0) != 0) {
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return obj;
   }

   obj = luabind::object(luabind::from_stack(L_, -1));
   lua_pop(L_, 1);
   return obj;
}

void ScriptHost::OnError(std::string description)
{
   LUA_LOG(0) << description;
}

luabind::object ScriptHost::Require(std::string const& s)
{
   std::string path, name;
   std::ostringstream script;

   std::vector<std::string> parts;
   boost::split(parts, s, boost::is_any_of("."));
   parts.erase(std::remove(parts.begin(), parts.end(), ""), parts.end());

   path = boost::algorithm::join(parts, "/") + ".lua";

   return RequireScript(path);
}

luabind::object ScriptHost::RequireScript(std::string const& path)
{
   res::ResourceManager2 const& rm = res::ResourceManager2::GetInstance();
   std::string canonical_path = rm.FindScript(path);

   luabind::object obj;

   auto i = required_.find(canonical_path);
   if (i != required_.end()) {
      obj = i->second;
   } else {
      LUA_LOG(5) << "requiring script " << canonical_path;
      required_[canonical_path] = luabind::object();
      obj = LoadScript(canonical_path);
      required_[canonical_path] = obj;
   }
   return obj;
}

#if 0
void ScriptHost::Call(luabind::object fn, luabind::object arg1)
{
   try {
      luabind::object caller(cb_thread_, fn);
      call_function<void>(caller, arg1);
   } catch (std::exception& e) {
      OnError(w.what());
   }

}
#endif

void ScriptHost::Log(const char* category, int level, const char* str)
{
   if (category && str) {
      LOG_CATEGORY_(level, BUILD_STRING("mod " << category)) << str;
   }
}

void ScriptHost::Exit(int code)
{
   LOG_(0) << "exiting with code " << code << " by mod request.";
   // NOOOOOOOOOOOO!  Can we gracefully shutdown?  So far this is used exclusively
   // by the autotest framework.
   TerminateProcess(GetCurrentProcess(), code);
}


uint ScriptHost::GetRealTime()
{
   return perfmon::Timer::GetCurrentTimeMs();
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
      config_level = core::Config::GetInstance().Get<int>(flag, SENTINEL);
      if (config_level != SENTINEL) {
         break;
      }
      flag = BUILD_STRING("logging.mods." << path << ".log_level");
      config_level = core::Config::GetInstance().Get<int>(flag, SENTINEL);
      if (config_level != SENTINEL) {
         break;
      }
      last = category.rfind('.', last - 1);
   }
   if (config_level == SENTINEL) {
      config_level = core::Config::GetInstance().Get<int>("logging.log_level", logger::GetDefaultLogLevel());
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

void ScriptHost::Trigger(const std::string& eventName)
{
   try {
      luabind::object radiant = globals(cb_thread_)["radiant"];
      radiant["events"]["trigger"](radiant, eventName);
   } catch (std::exception const& e) {
      ReportCStackThreadException(cb_thread_, e);
   }
}

void ScriptHost::TriggerOn(luabind::object obj, const std::string& eventName, luabind::object args)
{
   try {
      luabind::object radiant = globals(cb_thread_)["radiant"];
      radiant["events"]["trigger"](obj, eventName, args);
   } catch (std::exception const& e) {
      ReportCStackThreadException(cb_thread_, e);
   }
}

void ScriptHost::ClearMemoryProfile()
{
   if (enable_profile_memory_) {
      alloc_backmap.clear();
      alloc_map.clear();
      LOG_(0) << " cleared lua memory profile data";
   }
}

void ScriptHost::WriteMemoryProfile(std::string const& filename) const
{
   if (!enable_profile_memory_) {
      return;
   }
   if (!profile_memory_) {
      LOG_(0) << "memory profile is not running.";
      return;
   }

   std::map<int, std::pair<std::string, int>> totals;
   int grand_total = 0;
   unsigned int w = 0;
   for (const auto& entry : alloc_map) {
      int total = 0;
      for (const auto& alloc : entry.second) {
         total += alloc.second;
      }
      grand_total += total;
      totals[total] = std::make_pair(entry.first, entry.second.size());
      w = std::max(w, entry.first.size() + 2);
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

   output("Total Memory Tracked", 0, grand_total);
   output("Total Memory Allocated", 0, GetAllocBytesCount());
   for (auto i = totals.rbegin(); i != totals.rend(); i++) {
      output(i->second.first, i->second.second, i->first);
   }
   LOG_(0) << " wrote lua memory profile data to lua_memory_profile.txt";
}

void ScriptHost::ProfileMemory(bool value)
{
   if (enable_profile_memory_) {
      if (profile_memory_ && !value) {
         lua_sethook(L_, LuaTrackLine, 0, 1);
         ClearMemoryProfile();
      } else if (!profile_memory_ && value) {
         lua_sethook(L_, LuaTrackLine, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);
      }
      profile_memory_  = value;
      LOG_(0) << " lua memory profiling turned " << (profile_memory_ ? "on" : "off");
   }
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

luabind::object ScriptHost::StringToLua(std::string const& str)
{
   std::string eval = BUILD_STRING("return " << str);
   int ret = luaL_loadstring(L_, eval.c_str());
   if (ret != 0) {
      LOG_(0) << " error evaling string: " << eval;
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return luabind::object();
   }
   ret = lua_pcall(L_, 0, LUA_MULTRET, 0);
   if (ret != 0) {
      LOG_(0) << " error evaling string: " << eval;
      OnError(lua_tostring(L_, -1));
      lua_pop(L_, 1);
      return luabind::object();
   }

   luabind::object obj(luabind::from_stack(L_, -1));
   lua_pop(L_, 1);
   return obj;
}

luabind::object ScriptHost::GetObjectRepresentation(luabind::object obj, std::string const& format) const
{
   auto convert = [=](luabind::object& o) -> bool {
      switch (luabind::type(o)) {
      case LUA_TTABLE:
      case LUA_TUSERDATA:
         try {
            luabind::object __translator = o[format];
            if (__translator) {
               luabind::object translated = luabind::call_function<luabind::object>(__translator, o);
               if (translated) {
                  o = translated;
                  return true;
               }
            }
         } catch (std::exception& e) {
            LUA_LOG(1) << "call to " << format << " failed: " << e.what();
            ReportCStackThreadException(L_, e);
         }
      }
      return false;
   };

   luabind::object result(obj);
   while (convert(result)) {
      continue;
   }
   return result;
}

std::string ScriptHost::LuaToString(luabind::object obj)
{
   std::vector<luabind::object> visited;

   std::function<void(std::ostringstream&, luabind::object const&)> luaToString;

   luaToString = [&visited, &luaToString, this] (std::ostringstream& os, luabind::object const& current_obj) {
      luabind::object obj = GetObjectRepresentation(current_obj, "__repr");
      int t = luabind::type(obj);

      if (t == LUA_TTABLE) {
         for (luabind::object const& o : visited) {
            if (current_obj == o) {
               os << "\"visited node\"";
               return;
            }
         }
         visited.push_back(current_obj);
      }

      if (obj != current_obj && t == LUA_TSTRING) {
         // don't modify it.. it's a lua string meant to be evaled.
         os << object_cast<std::string>(obj);
      } else {
         if (t == LUA_TTABLE) {
            os << "{ ";
            for (iterator i(obj), end; i != end; i++) {
               os << "[";
                  luaToString(os, i.key());
               os << "] = ";
                  luaToString(os, *i);
               os << ", ";
            }
            os << " }";
         } else if (t == LUA_TUSERDATA) {
            class_info ci = call_function<class_info>(globals(L_)["class_info"], obj);
            throw std::logic_error(BUILD_STRING("lua userdata object of type " << ci.name << " does not implement __repr"));
         } else if (t == LUA_TSTRING) {
            std::string str = object_cast<std::string>(obj);
            boost::algorithm::replace_all(str, "'", "\\'");
            os << "'" << str << "'";
         } else if (t == LUA_TNUMBER) {
            std::ostringstream formatter;
            double double_value = object_cast<double>(obj);
            int int_value = static_cast<int>(double_value);
            if (csg::IsZero(double_value - int_value)) {
               os << int_value;
            } else {
               os << double_value;
            }
         } else if (t == LUA_TBOOLEAN) {
            os << (object_cast<bool>(obj) ? "true" : "false");
         } else if (t == LUA_TFUNCTION) {
            os << "\"function\"";
         }
      }
   };

   std::ostringstream buffer;
   luaToString(buffer, obj);

   std::string repr = buffer.str();
   return repr;
}

void ScriptHost::AddObjectToLuaConvertor(dm::ObjectType type, ObjectToLuaFn cast_fn)
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
   object __numeric = tbl["__numeric"];
   if (__numeric.is_valid() && type(__numeric) == LUA_TBOOLEAN && object_cast<bool>(__numeric)) {
      return true;
   }
   if (i == end) {
      return false; // assume empty tables are objects.
   }
   return luabind::type(tbl) == LUA_TTABLE && luabind::type(tbl[1]) != LUA_TNIL;
}
