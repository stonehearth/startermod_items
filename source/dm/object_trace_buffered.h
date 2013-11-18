#ifndef _RADIANT_DM_OBJECT_TRACE_BUFFERED_H
#define _RADIANT_DM_OBJECT_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "object_trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
class ObjectTraceBuffered : public ObjectTrace<T>,
                            public TraceBuffered
                            
{
public:
   ObjectTraceBuffered(const char* reason) :
      ObjectTrace(reason)
   {
   }

   void Flush()
   {
      if (changed_) {
         SignalChanged();
      }
   }

private:
   void NotifyObjectChanged()
   {
      changed_ = true;
   }

private:
   bool        changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_BUFFERED_H

