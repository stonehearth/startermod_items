#ifndef _RADIANT_LUA_FLAME_GRAPH_H
#define _RADIANT_LUA_FLAME_GRAPH_H

#include "lib/lua/bind.h"
#include "core/static_string.h"

BEGIN_RADIANT_LUA_NAMESPACE

class FunctionLineInfo {
public:
   FunctionLineInfo() : function(""), min(0), max(0) { }
   FunctionLineInfo(std::string const& fn, int a, int b) : function(fn), min(a), max(b) { }

public:
   core::StaticString function;
   int min, max;
};

class FileLineInfo {
public:
   std::vector<FunctionLineInfo> functions;
};

class LuaFlameGraph {
public:
   LuaFlameGraph(lua_State* L, ScriptHost& sh);

   core::StaticString MapFileLineToFunction(core::StaticString file, int line);
   void IndexFile(core::StaticString file);
   bool IsIndexing() { return _indexing; }

public:
   typedef std::unordered_map<core::StaticString, FileLineInfo, std::hash<const char*>> FileIndex;

   lua_State*           _L;
   ScriptHost&          _sh;
   bool                 _indexing;
   luabind::object      _flameGraphObj;
   luabind::object      _mapSourceFunctionsFn;
   FileIndex            _files;
};


END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_FLAME_GRAPH_H