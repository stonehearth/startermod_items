#include "radiant.h"
#include "array.h"
#include "array_trace_buffered.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename A>
ArrayTraceBuffered<A>::ArrayTraceBuffered(const char* reason, Object const& o, Store const& store) :
   TraceBuffered(reason, o, store),
   ArrayTrace(reason, o, store)
{
}

template <typename A>
void ArrayTraceBuffered<A>::Flush()
{
   SignalUpdated(changed_);
   changed_.clear();
}

template <typename A>
void ArrayTraceBuffered<A>::SaveObjectDelta(Object* obj, Protocol::Value* value)
{
   for (const auto& entry : changed_) {
      Protocol::Array::Entry* msg = valmsg->AddExtension(Protocol::Array::extension);
      msg->set_index(entry.first);
      SaveImpl<T>::SaveValue(store, msg->mutable_value(), entry.second);
   }
}

template <typename A>
void ArrayTraceBuffered<A>::NotifySet(uint i, Value const& key)
{
   changed_[i] = key;
}
