#ifndef _RADIANT_DM_OBJECT_TRACE_H
#define _RADIANT_DM_OBJECT_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTrace : public Trace
{
public:
   ObjectTrace(const char* reason) : Trace(reason) { }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_H

