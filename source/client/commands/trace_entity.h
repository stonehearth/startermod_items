#ifndef _RADIANT_CLIENT_TRACE_ENTITY_H
#define _RADIANT_CLIENT_TRACE_ENTITY_H

#include "pending_command.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class TraceEntityCmd
{
public:
   TraceEntityCmd() {};
   void operator()(PendingCommandPtr cmd);

private:
   void TraceEntity(om::EntityId id, const JSONNode& components, JSONNode& result);
};

class TraceEntitiesCmd
{
public:
   TraceEntitiesCmd() {};
   void operator()(PendingCommandPtr cmd);
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_TRACE_ENTITY_H
