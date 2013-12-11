#ifndef _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename T>
class BoxedTraceWrapper : public std::enable_shared_from_this<BoxedTraceWrapper<T>>
{
public:
   BoxedTraceWrapper(std::shared_ptr<T> trace);
   ~BoxedTraceWrapper();

   std::shared_ptr<BoxedTraceWrapper<T>> OnDestroyed(lua_State* L, luabind::object destroyed_cb);
   std::shared_ptr<BoxedTraceWrapper<T>> OnChanged(lua_State* L, luabind::object changed_cb);
   std::shared_ptr<BoxedTraceWrapper<T>> PushObjectState();
   void Destroy();
   
private:
   std::shared_ptr<T>   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_BOXED_TRACE_WRAPPER_H