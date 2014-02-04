#ifndef _RADIANT_LUA_DM_SET_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_SET_TRACE_WRAPPER_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename T>
class SetTraceWrapper : public std::enable_shared_from_this<SetTraceWrapper<T>>
{
public:
   SetTraceWrapper(std::shared_ptr<T> trace);

   std::shared_ptr<SetTraceWrapper<T>> OnDestroyed(luabind::object destroyed_cb);
   std::shared_ptr<SetTraceWrapper<T>> OnAdded(luabind::object added_cb);
   std::shared_ptr<SetTraceWrapper<T>> OnRemoved(luabind::object removed_cb);
   std::shared_ptr<SetTraceWrapper<T>> PushObjectState();
   void Destroy();
   
private:
   std::shared_ptr<T>   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_SET_TRACE_WRAPPER_H