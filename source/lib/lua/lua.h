#ifndef _RADIANT_LIB_LUA_LUA_H
#define _RADIANT_LIB_LUA_LUA_H

#define BEGIN_RADIANT_LUA_NAMESPACE  namespace radiant { namespace lua {
#define END_RADIANT_LUA_NAMESPACE    } }

BEGIN_RADIANT_LUA_NAMESPACE

class ScriptHost;
class TraceWrapper;

#define DEFINE_INVALID_LUA_CONVERSION(T) \
   template <> std::string lua::Repr(T const&) { \
      throw std::invalid_argument(BUILD_STRING("cannot encode lua userdata object of type " << GetShortTypeName<T>() << " as string")); \
   }

template <typename T> std::string Repr(T const& obj);

#define LUA_LOG(level)     LOG(lua.code, level)

DECLARE_SHARED_POINTER_TYPES(TraceWrapper);

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LIB_LUA_LUA_H
