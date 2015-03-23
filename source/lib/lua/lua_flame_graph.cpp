#include "pch.h"
#include <stdexcept>
#include "radiant_file.h"
#include "lua_flame_graph.h"
#include "script_host.h"

using namespace ::radiant;
using namespace ::radiant::lua;

LuaFlameGraph::LuaFlameGraph(lua_State* L, ScriptHost& sh) :
   _L(L),
   _sh(sh),
   _indexing(false)
{
   lua_getfield(L, LUA_GLOBALSINDEX, "require");
   lua_pushstring(L, "coverage.flame_graph");
   lua_call(L, 1, 1);

   _flameGraphObj = luabind::object(luabind::from_stack(_L, -1));
   _mapSourceFunctionsFn = _flameGraphObj["map_source_function"];

   lua_pop(L, 1);
}

core::StaticString LuaFlameGraph::MapFileLineToFunction(core::StaticString file, int line)
{
   auto i = _files.find(file);
   if (i != _files.end()) {
      FileLineInfo& fi = i->second;
      for (FunctionLineInfo const& fli : fi.functions) {
         if (line >= fli.min && line < fli.max) {
            return fli.function;
         }
      }
   }
   return "unknown";
}

void LuaFlameGraph::IndexFile(core::StaticString filename)
{
   auto res = _files.emplace(filename, FileLineInfo());
   FileLineInfo& fi = res.first->second;

   if (!res.second) {
      return;
   }

   LOG(script_host, 0) << "indexing " << filename;
   _indexing = true;

   try {
      auto& rm = res::ResourceManager2::GetInstance();
      auto stream = rm.OpenResource(std::string(filename));
      if (stream) {
         std::string contents = io::read_contents(*stream);
         luabind::object mapping = _mapSourceFunctionsFn(_flameGraphObj, contents, (const char*)filename);
         luabind::iterator i(mapping), end;
         for (; i != end; i++) { 
            fi.functions.emplace_back(
               luabind::object_cast<std::string>((*i)[1]),
               luabind::object_cast<int>((*i)[2]),
               luabind::object_cast<int>((*i)[3]));
         }
      }
   } catch (std::exception const& e) {
      _sh.ReportCStackThreadException(_L, e);
   }

   _indexing = false;
}

