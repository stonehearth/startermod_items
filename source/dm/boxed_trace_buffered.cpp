#include "radiant.h"
#include "boxed.h"
#include "boxed_trace_buffered.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

#define TRACE_LOG(level)  LOG_CATEGORY(dm.trace.boxed, level, "buf boxed<" << GetShortTypeName<B::Value>() << "> " << GetReason())

template <typename B>
BoxedTraceBuffered<B>::BoxedTraceBuffered(const char* reason, B const& b) :
   BoxedTrace(reason, b),
   value_(nullptr)
{
   TRACE_LOG(5) << "creating trace for object " << GetObjectId();
}

template <typename B>
void BoxedTraceBuffered<B>::Flush()
{
   TRACE_LOG(5) << "flushing trace for object " << GetObjectId();
   if (value_) {
      // xxx: this might be an old value since it points back into the box (ug!)
      SignalChanged(*value_);
      ClearCachedState();
   }
}

template <typename B>
bool BoxedTraceBuffered<B>::SaveObjectDelta(SerializationType r, Protocol::Value* value)
{
   TRACE_LOG(5) << "saving trace for object " << GetObjectId();
   if (value_) {
      SaveImpl<B::Value>::SaveValue(GetStore(), r, value, *value_);
      return true;
   }
   return false;
}

template <typename B>
void BoxedTraceBuffered<B>::NotifyChanged(Value const& value)
{
   value_ = &value;
}

template <typename B>
void BoxedTraceBuffered<B>::ClearCachedState()
{
   value_ = nullptr;
}

#define CREATE_BOXED(B)           template BoxedTraceBuffered<B>;
#include "types/all_loader_types.h"
#include "types/all_boxed_types.h"
ALL_DM_BOXED_TYPES
