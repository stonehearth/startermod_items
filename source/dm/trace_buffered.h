#ifndef _RADIANT_DM_TRACE_BUFFERED_H
#define _RADIANT_DM_TRACE_BUFFERED_H

#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

class TraceBuffered
{
public:
   virtual void Flush() = 0;
   virtual void SaveObjectDelta(Protocol::Value* value) = 0;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_BUFFERED_H
