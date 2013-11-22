#ifndef _RADIANT_DM_RECORD_TRACE_BUFFERED_H
#define _RADIANT_DM_RECORD_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "record_trace.h"
#include "record_loader.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class RecordTraceBuffered : public RecordTrace<M>,
                            public TraceBuffered
{
public:
   RecordTraceBuffered(const char* reason, Record const& r, Store const& s, int category) :
      RecordTrace(reason, r, s, category),
      first_save_(true),
      changed_(false)
   {
   }

   void Flush() override
   {
      ASSERT(changed_);
      if (changed_) {
         SignalModified();
         changed_ = false;
      }
   }

   void SaveObjectDelta(Object* obj, Protocol::Value* value) override
   {
      if (first_save_) {
         SaveObject(*static_cast<Record*>(obj), value);
         first_save_ = false;
      }
   }

private:
   void NotifyRecordChanged() override
   {
      changed_ = true;
   }

private:
   bool        changed_;
   bool        first_save_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_TRACE_BUFFERED_H

