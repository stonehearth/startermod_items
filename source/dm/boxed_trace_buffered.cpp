#include "radiant.h"
#include "boxed.h"
#include "boxed_trace_buffered.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename B>
BoxedTraceBuffered<B>::BoxedTraceBuffered(const char* reason, Object const& o, Store const& store) :
   BoxedTrace(reason, o, store),
   value_(nullptr)
{
}

template <typename B>
void BoxedTraceBuffered<B>::Flush()
{
   ASSERT(value_);
   if (value_) {
      // xxx: this might be an old value since it points back into the box (ug!)
      SignalChanged(*value_);
      value_ = nullptr;
   }
}

template <typename B>
void BoxedTraceBuffered<B>::SaveObjectDelta(Object* obj, Protocol::Value* value)
{
   ASSERT(value_);
   if (value_) {
      SaveImpl<B::Value>::SaveValue(GetStore(), value, *value_);
   }
}

template <typename B>
void BoxedTraceBuffered<B>::NotifyChanged(Value const& value)
{
   value_ = &value;
}

template <typename B>
void BoxedTraceBuffered<B>::NotifyObjectState(Value const& value)
{
   value_ = nullptr;
   BoxedTrace<B>::NotifyObjectState(value);
}

#define CREATE_BOXED(B)           template BoxedTraceBuffered<B>;
#include "types/all_loader_types.h"
#include "types/all_boxed_types.h"
ALL_DM_BOXED_TYPES
