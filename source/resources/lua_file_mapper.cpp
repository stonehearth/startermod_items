#include "radiant.h"
#include "lua_file_mapper.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::res;

#pragma optimize ( "", off )

LuaFileMapper::LuaFileMapper(lua::ScriptHost& sh) :
   _sh(sh)
{
   try {
      _flameGraphObj = sh.Require("radiant.lib.compiler.file_mapper");
      _mapSourceFunctionsFn = _flameGraphObj["map_source_function"];
   } catch (std::exception const& e) {
      _sh.ReportCStackException(_sh.GetInterpreter(), e);
   }
}

core::StaticString LuaFileMapper::MapFileLineToFunction(core::StaticString file, int line)
{
   auto i = _files.find(file);
   if (i != _files.end()) {
      FileLineInfo& fi = i->second;
      ASSERT(line >= 0 && line < (int)fi.lineToFunction.size());
      return core::StaticString::FromPreviousStaticStringValue(fi.lineToFunction[line]);
   }
   return file;
}

void LuaFileMapper::IndexFile(core::StaticString filename, std::string const& contents)
{
   auto res = _files.emplace(filename, FileLineInfo());
   FileLineInfo& fi = res.first->second;

   if (!res.second) {
      return;
   }

   LOG(script_host, 0) << "indexing " << filename;

   try {
      luabind::object mapping = _mapSourceFunctionsFn(_flameGraphObj, contents, (const char*)filename);
      luabind::iterator i(mapping), end;
      for (; i != end; i++) {
         core::StaticString fn = luabind::object_cast<std::string>((*i)[1]);
         int min = luabind::object_cast<int>((*i)[2]);
         int max = luabind::object_cast<int>((*i)[3]);
         if ((int)fi.lineToFunction.size() <= max) {
            fi.lineToFunction.resize(max + 1);
         }
         for (int i = min; i <= max; i++) {
            fi.lineToFunction[i] = fn;
         }
      }
   } catch (std::exception const& e) {
      _sh.ReportCStackException(_mapSourceFunctionsFn.interpreter(), e);
   }
}

