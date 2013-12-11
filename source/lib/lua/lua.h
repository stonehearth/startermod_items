#ifndef _RADIANT_LIB_LUA_LUA_H
#define _RADIANT_LIB_LUA_LUA_H

#define BEGIN_RADIANT_LUA_NAMESPACE  namespace radiant { namespace lua {
#define END_RADIANT_LUA_NAMESPACE    } }

BEGIN_RADIANT_LUA_NAMESPACE

class ScriptHost;
class TraceWrapper;

#define LUA_LOG(level)     LOG(lua.code, level)

DECLARE_SHARED_POINTER_TYPES(TraceWrapper);

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LIB_LUA_LUA_H
