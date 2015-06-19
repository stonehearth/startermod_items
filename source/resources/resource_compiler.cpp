#include "radiant.h"
#include "res_manager.h"
#include "resource_compiler.h"
#include "lib/lua/script_host.h"
#include "lua_file_mapper.h"
#include "core/config.h"

using namespace ::radiant;
using namespace ::radiant::res;

class LuaStringReader
{
public:
   LuaStringReader(std::string const& s) : _s(s), _finished(false) { }

   static const char *LoadFn(lua_State *L, void *data, size_t *size) {
      return ((LuaStringReader *)data)->Load(L, data, size);
   }

private:
   const char *Load(lua_State *L, void *data, size_t *size) {
      if (_finished) {
         *size = 0;
         return nullptr;
      }
      *size = _s.size();
      _finished = true;
      return _s.c_str();
   }
   
private:
   std::string const&   _s;
   bool                 _finished;
};

ResourceCompiler::ResourceCompiler()
{
}

void ResourceCompiler::Initialize()
{
   _scriptHost.reset(new lua::ScriptHost("compiler"));
   lua_State* L = _scriptHost->GetInterpreter();

   // The act of requiring these scripts will loop back around
   // and end up compiling even more scripts!
   _scriptHost->Require("radiant.lib.env");
   //_scriptHost->Require("radiant.lib.strict");

   bool cpuProfilerEnabled = core::Config::GetInstance().Get<bool>("lua.enable_cpu_profiler", false);
   if (cpuProfilerEnabled) {
      _luaFileMapper.reset(new LuaFileMapper(*_scriptHost));
   }
}

ResourceCompiler::~ResourceCompiler()
{
   _luaFileMapper.reset();
   _scriptHost.reset();
}

luabind::object ResourceCompiler::CompileScript(lua_State* L, std::string const& path, std::string const& contents)
{
   luabind::object obj;
   LuaStringReader reader(contents);
   core::StaticString module = std::string("@") + path;

   int error = lua_load(L, LuaStringReader::LoadFn, &reader, module);
   if (error != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 1);
      throw std::logic_error(BUILD_STRING("Error loading \"" << path << "\": " << error));
   }

   if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
      std::string error = lua_tostring(L, -1);
      lua_pop(L, 1);
      throw std::logic_error(BUILD_STRING("Error loading \"" << path << "\": " << error));
   }

   obj = luabind::object(luabind::from_stack(L, -1));
   lua_pop(L, 1);
   
   if (_luaFileMapper) {
      _luaFileMapper->IndexFile(module, contents);
   }

   return obj;
}

LuaFunctionInfo ResourceCompiler::MapFileLineToFunction(core::StaticString file, int line)
{
   ASSERT(_luaFileMapper);
   if (_luaFileMapper) {
      return _luaFileMapper->MapFileLineToFunction(file, line);
   }
   LuaFunctionInfo result;
   result.functionName = "cpu profiler not enabled";
   result.startLine = 0;
   result.endLine = 0;
   return result;
}
