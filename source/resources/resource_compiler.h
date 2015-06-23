#ifndef _RADIANT_RES_RESOURCE_COMPILER_H
#define _RADIANT_RES_RESOURCE_COMPILER_H

#include "namespace.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_RES_NAMESPACE

class ResourceCompiler
{
public:
   ResourceCompiler();
   ~ResourceCompiler();

   void Initialize();
   luabind::object CompileScript(lua_State* L, std::string const& path, std::string const& contents);
   LuaFunctionInfo MapFileLineToFunction(core::StaticString file, int line);

private:
   std::unique_ptr<lua::ScriptHost>    _scriptHost;
   std::unique_ptr<LuaFileMapper>      _luaFileMapper;
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_RESOURCE_MANAGER_H
