#ifndef _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename T>
class MapTraceWrapper : public std::enable_shared_from_this<MapTraceWrapper<T>>
{
public:
   MapTraceWrapper(std::shared_ptr<T> trace);

   std::shared_ptr<MapTraceWrapper<T>> OnDestroyed(luabind::object destroyed_cb);
   std::shared_ptr<MapTraceWrapper<T>> OnAdded(luabind::object changed_cb);
   std::shared_ptr<MapTraceWrapper<T>> OnRemoved(luabind::object removed_cb);
   std::shared_ptr<MapTraceWrapper<T>> PushObjectState();
   void Destroy();

private:
   std::shared_ptr<T>   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_MAP_TRACE_WRAPPER_H