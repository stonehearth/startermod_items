#include "pch.h"
#include "script_host.h"
#include "client/renderer/render_entity.h"

extern "C" int luaopen_lpeg (lua_State *L);

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::lua;

static int PCallCallbackFn(lua_State* L)
{
   using namespace luabind;

   if (!lua_isstring(L, 1)) { /* 'message' not a string? */
      return 1;  /* keep it intact */
   }
   std::string last_error = lua_tostring(L, 1);

   LOG(WARNING) << lua_tostring(L, 1);
   lua_getfield(L, LUA_GLOBALSINDEX, "debug");
   if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      LOG(WARNING) << "aborting traceback.  expected table for debug.";
      return 1;
   }
   lua_getfield(L, -1, "traceback");
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 2);
      LOG(WARNING) << "aborting traceback.  expected function for traceback.";
      return 1;
   }
   LOG(WARNING) << "generating traceback...";
   lua_call(L, 0, 1);  /* call debug.traceback */
   std::string lastTraceback_ = std::string(lua_tostring(L, -1));
   lua_pop(L, 1);

   ScriptHost* sh = object_cast<ScriptHost*>(globals(L)["script_host"]);
   sh->NotifyError(last_error, lastTraceback_);

   return 1;
}

luabind::object ScriptHost::JsonToLua(JSONNode const& json)
{
   using namespace luabind;

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
   }
   ASSERT(false);
   return object();
}

ScriptHost::ScriptHost()
{
   L_ = lua_newstate(LuaAllocFn, this);
	luaL_openlibs(L_);
   luaopen_lpeg(L_);

   luabind::open(L_);
   luabind::bind_class_info(L_);

   set_pcall_callback(PCallCallbackFn);

   module(L_) [
      namespace_("_radiant") [
         namespace_("lua") [
            class_<ScriptHost>("ScriptHost")
               .def("lua_require",     &ScriptHost::LuaRequire)
               .def("log",             &ScriptHost::Log)
               .def("assert_failed",   &ScriptHost::AssertFailed)
         ]
      ]
   ];
   globals(L_)["native"] = object(L_, this);

   globals(L_)["package"]["path"] = "./data/?.lua";
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

void ScriptHost::NotifyError(std::string const& error, std::string const& traceback)
{
   lastError_ = error;
   lastTraceback_ = traceback;
}

lua_State* ScriptHost::GetInterpreter()
{
   return L_;
}

lua_State* ScriptHost::GetCallbackState()
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

   LOG(WARNING) << "loading script " << path;
   // this is the slightly crappier version
   std::string filepath;
   try {
       filepath = resources::ResourceManager2::GetInstance().GetResourceFileName(path, ".lua");
   } catch (resources::Exception& e) {
      LOG(WARNING) << e.what();
	   return obj;
   }

   int error = luaL_loadfile(L_, filepath.c_str());
   if (error == LUA_ERRFILE) {
      OnError("Coud not open script file " + filepath);
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
   LOG(WARNING) << "-- Lua Error Begin ------------------------------- ";
   std::string item;
   std::stringstream ss(description);
   while(std::getline(ss, item)) {
      LOG(WARNING) << "   " << item;
   }
   if (!lastError_.empty()) {
      LOG(WARNING) << lastError_;

      std::stringstream ss(lastTraceback_);
      while(std::getline(ss, item)) {
         LOG(WARNING) << "   " << item;
      }
      lastError_.clear();
      lastTraceback_.clear();
   }
   LOG(WARNING) << "-- Lua Error End   ------------------------------- ";

#if 0
   luabind::object tb = luabind::globals(L_)["debug"]["traceback"];
   if (tb.is_valid()) { 
      std::string traceback = call_function<std::string>(tb);
      LOG(WARNING) << lua_tostring(L_, -1);
      LOG(WARNING) << "-- stack:";
      LOG(WARNING) << traceback;
      LOG(WARNING) << "-- endstack";
   }
#endif
#if 1
   int startTime = timeGetTime();
   while(timeGetTime() - startTime < 1500000) {
      Sleep(0);
      //DebugBreak();
   }
#endif
}

luabind::object ScriptHost::LuaRequire(std::string uri)
{
   std::string path, name;
   std::ostringstream script;
   luabind::object obj;

   std::string canonical_path;
   try {
      canonical_path = resources::ResourceManager2::GetInstance().ConvertToCanonicalPath(uri, ".lua");
   } catch (resources::Exception& e) {
      LOG(WARNING) << e.what();
      return obj;
   }
   std::string key = canonical_path;

   auto i = required_.find(key);
   if (i != required_.end()) {
      obj = i->second;
   } else {
      obj = LoadScript(canonical_path);
      required_[key] = obj;
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

void ScriptHost::Log(std::string str)
{
   LOG(WARNING) << str;
}

void ScriptHost::AssertFailed(std::string reason)
{
   lua_pushstring(L_, reason.c_str());
   lua_insert(L_, 1);
   PCallCallbackFn(L_);
   OnError("Lua assert failed");
}
