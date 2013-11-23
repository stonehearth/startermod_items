#include "radiant.h"
#include "radiant_stdutil.h"
#include "set.h"
#include "set_trace_buffered.h"

using namespace radiant;
using namespace radiant::dm;

template <typename S>
SetTraceBuffered<S>::SetTraceBuffered(const char* reason, Object const& o, Store const& store) :
   SetTrace(reason, o, store)
{
}

template <typename S>
void SetTraceBuffered<S>::Flush()
{
   SignalUpdated(added_, removed_);
   added_.clear();
   removed_.clear();
}

template <typename S>
void SetTraceBuffered<S>::SaveObjectDelta(Object* obj, Protocol::Value* value)
{
   Protocol::Set::Update* msg = value->MutableExtension(Protocol::Set::extension);
   for (const Value& item : added_) {
      Protocol::Value* submsg = msg->add_added();
      SaveImpl<Value>::SaveValue(GetStore(), submsg, item);
   }
   for (const Value& item : removed_) {
      Protocol::Value* submsg = msg->add_removed();
      SaveImpl<Value>::SaveValue(GetStore(), submsg, item);
   }
}

template <typename S>
void SetTraceBuffered<S>::NotifyAdded(Value const& value)
{
   stdutil::FastRemove(removed_, value);
   added_.push_back(value);
}

template <typename S>
void SetTraceBuffered<S>::NotifyRemoved(Value const& value)
{
   stdutil::FastRemove(added_, value);
   removed_.push_back(value);
}

template <typename S>
void SetTraceBuffered<S>::NotifyObjectState(typename S::ContainerType const& contents)
{
   added_.clear();
   removed_.clear();
   SetTrace<S>::NotifyObjectState(contents);
}

#define CREATE_SET(S)  template SetTraceBuffered<S>;
#include "types/instantiate_types.h"

