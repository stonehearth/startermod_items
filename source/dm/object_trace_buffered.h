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
   ObjectTraceBuffered(const char* reason, Object const& o, Store const& store) :
      ObjectTrace(reason, o, store)
   {
   }

   void Flush()
   {
      if (changed_) {
         SignalModified();
         changed_ = false;
      }
   }

   void SaveObjectDelta(Object* obj, Protocol::Value* value) override
   {
      throw std::logic_error("cannot save state of non-specific object!");
   }

private:
   void NotifyObjectState() override
   {
      changed_ = false;
      ObjectTrace::PushObjectState();
   }

private:
   bool        changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_TRACE_BUFFERED_H

