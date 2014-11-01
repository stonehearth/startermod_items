#ifndef _RADIANT_DM_TRACE_BUFFERED_LIST_H
#define _RADIANT_DM_TRACE_BUFFERED_LIST_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class TraceBufferedList
{
public:
   TraceBufferedList();

   void AddTrace(TraceBufferedRef t);
   void ForEachTrace(std::function<void(TraceBufferedPtr)> cb);

   bool IsEmpty() const;

private:
   bool                             _firing;
   std::vector<TraceBufferedRef>    _traces;
   std::vector<TraceBufferedRef>    _pendingTraces;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_BUFFERED_LIST_H

