#include "pch.h"
#include <stdexcept>
#include "radiant_file.h"
#include "core/config.h"
#include "script_host.h"
#include "lua_supplemental.h"
#include "client/renderer/render_entity.h"
#include "lib/json/namespace.h"

extern "C" int luaopen_lpeg (lua_State *L);

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

DEFINE_INVALID_JSON_CONVERSION(ScriptHost);

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

void ScriptHost::AddJsonToLuaConverter(JsonToLuaFn fn)
{
   to_lua_converters_.emplace_back(fn);
}

JSONNode ScriptHost::LuaToJson(luabind::object obj)
{
   int t = type(obj);
   if (t == LUA_TTABLE || t == LUA_TUSERDATA) {
      try {
         object coder = globals(L_)["radiant"]["json"];
         std::string json = call_function<std::string>(coder["encode"], obj);
         return libjson::parse(json);
      } catch (std::exception& e) {
         LUA_LOG(1) << "failed to convert coded json string to node: " << e.what();
         ReportCStackThreadException(obj.interpreter(), e);
         return JSONNode();
      }
   } else if (t == LUA_TSTRING) {
      return JSONNode("", object_cast<std::string>(obj));
   } else if (t == LUA_TNUMBER) {
      std::ostringstream formatter;
      float value = object_cast<float>(obj);
      int int_value = static_cast<int>(value);
      if (csg::IsZero(value - int_value)) {
         return JSONNode("", int_value);
      }
      return JSONNode("", value);
   } else if (t == LUA_TBOOLEAN) {
      return JSONNode("", object_cast<bool>(obj));
   }
   LUA_LOG(1) << "unknown type converting lua to json: " << t;
   return JSONNode();
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
   json::Node json = res::ResourceManager2::GetInstance().LookupJson(uri);
   return JsonToLua(json.get_internal_node());
}

res::AnimationPtr ScriptHost_LoadAnimation(std::string uri)
{
   return res::ResourceManager2::GetInstance().LookupAnimation(uri);
}

IMPLEMENT_TRIVIAL_TOSTRING(ScriptHost);


ScriptHost::ScriptHost()
{
   filter_c_exceptions_ = core::Config::GetInstance().Get<bool>("lua.filter_exceptions", true);

   L_ = lua_newstate(LuaAllocFn, this);
	luaL_openlibs(L_);
   luaopen_lpeg(L_);

   luabind::open(L_);
   luabind::bind_class_info(L_);

   set_pcall_callback(PCallCallbackFn);

   module(L_) [
      namespace_("_radiant") [
         namespace_("lua") [
            lua::RegisterType<ScriptHost>()
               .def("log",             &ScriptHost::Log)
               .def("get_log_level",   &ScriptHost::GetLogLevel)
               .def("report_error",    (void (ScriptHost::*)(std::string const& error, std::string const& traceback))&ScriptHost::ReportLuaStackException)
               .def("require",         (luabind::object (ScriptHost::*)(std::string const& name))&ScriptHost::Require)
               .def("require_script",  (luabind::object (ScriptHost::*)(std::string const& name))&ScriptHost::RequireScript)
         ]
      ]
   ];
   globals(L_)["_host"] = object(L_, this);
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

void* ScriptHost::LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize)
{
   if (nsize == 0) {
      free(ptr);
      return NULL;
   }
   return realloc(ptr, nsize);
}


void ScriptHost::ReportCStackThreadException(lua_State* L, std::exception const& e)
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
   ReportStackException("lua", error, traceback);
}

void ScriptHost::ReportStackException(std::string const& category, std::string const& error, std::string const& traceback)
{
   LUA_LOG(0) << "-- Script Error (" << category << ") Begin ------------------------------- ";
   if (!error.empty()) {
      LUA_LOG(0) << error;

      std::string item;
      std::stringstream ss(traceback);
      while(std::getline(ss, item)) {
         LUA_LOG(0) << "   " << item;
      }
   }
   LUA_LOG(0) << "-- Lua Error End   ------------------------------- ";
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
   LOG_CATEGORY_(level, BUILD_STRING("mod " << category)) << str;
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
      config_level = core::Config::GetInstance().Get<int>("logging.log_level", SENTINEL);
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
