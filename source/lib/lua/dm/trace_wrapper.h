#ifndef _RADIANT_LUA_DM_TRACE_WRAPPER_H
#define _RADIANT_LUA_DM_TRACE_WRAPPER_H

#include "lib/lua/lua.h"
#include "dm/trace.h"

BEGIN_RADIANT_LUA_NAMESPACE

class TraceWrapper : public std::enable_shared_from_this<TraceWrapper>
{
public:
   TraceWrapper(dm::TracePtr trace);

   std::shared_ptr<TraceWrapper> OnChanged(luabind::object changed_cb);
   std::shared_ptr<TraceWrapper> OnDestroyed(luabind::object destroyed_cb);
   std::shared_ptr<TraceWrapper> PushObjectState();
   void Destroy();
   
private:
   dm::TracePtr   trace_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_TRACE_WRAPPER_H