#ifndef _RADIANT_RES_LUA_FILE_MAPPER_H
#define _RADIANT_RES_LUA_FILE_MAPPER_H

#include <vector>
#include "namespace.h"
#include "lib/lua/bind.h"
#include "core/static_string.h"

BEGIN_RADIANT_RES_NAMESPACE

class FileLineInfo {
public:
   std::vector<const char*> lineToFunction;
};

class LuaFileMapper {
public:
   LuaFileMapper(lua::ScriptHost& sh);

   core::StaticString MapFileLineToFunction(core::StaticString file, int line);
   void IndexFile(core::StaticString filename, std::string const& contents);

public:
   typedef std::unordered_map<core::StaticString, FileLineInfo, std::hash<const char*>> FileIndex;

   lua::ScriptHost&     _sh;
   luabind::object      _flameGraphObj;
   luabind::object      _mapSourceFunctionsFn;
   FileIndex            _files;
};


END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_LUA_FILE_MAPPER_H