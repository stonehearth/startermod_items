#include "radiant.h"
#include "radiant_stdutil.h"
#include "set.h"
#include "set_trace_buffered.h"

using namespace radiant;
using namespace radiant::dm;

template <typename S>
SetTraceBuffered<S>::SetTraceBuffered(const char* reason, S const& set) :
   SetTrace(reason, set)
{
}

template <typename S>
void SetTraceBuffered<S>::Flush()
{
   SignalUpdated(added_, removed_);
   ClearCachedState();
}

template <typename S>
bool SetTraceBuffered<S>::SaveObjectDelta(Protocol::Value* value)
{
   Store const& store = GetStore();
   Protocol::Set::Update* msg = value->MutableExtension(Protocol::Set::extension);

   if (!added_.empty() || !removed_.empty()) {
      for (const Value& item : added_) {
         Protocol::Value* submsg = msg->add_added();
         SaveImpl<Value>::SaveValue(store, submsg, item);
      }
      for (const Value& item : removed_) {
         Protocol::Value* submsg = msg->add_removed();
         SaveImpl<Value>::SaveValue(store, submsg, item);
      }
      return true;
   }
   return false;
}

template <typename S>
void SetTraceBuffered<S>::ClearCachedState()
{
   added_.clear();
   removed_.clear();
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

#define CREATE_SET(S)  template SetTraceBuffered<S>;
#include "types/all_set_types.h"
#include "types/all_loader_types.h"
ALL_DM_SET_TYPES
