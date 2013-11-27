#include "radiant.h"
#include "set.h"
#include "set_trace.h"
#include "store.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename S>
SetTrace<S>::SetTrace(const char* reason, S const& set) :
   TraceImpl(reason),
   set_(set)
{
}

template <typename S>
ObjectId SetTrace<S>::GetObjectId() const
{
   return set_.GetObjectId();
}

template <typename S>
Store const& SetTrace<S>::GetStore() const
{
   return set_.GetStore();
}

template <typename S>
std::shared_ptr<SetTrace<S>> SetTrace<S>::OnAdded(AddedCb added)
{
   added_ = added;
   return shared_from_this();
}

template <typename S>
std::shared_ptr<SetTrace<S>> SetTrace<S>::OnRemoved(RemovedCb removed)
{
   removed_ = removed;
   return shared_from_this();
}

template <typename S>
std::shared_ptr<SetTrace<S>> SetTrace<S>::OnUpdated(UpdatedCb updated)
{
   updated_ = updated;
   return shared_from_this();
}

template <typename S>
void SetTrace<S>::SignalObjectState()
{
   SignalUpdated(set_.GetContainer(), ValueList());
}

template <typename S>
void SetTrace<S>::SignalRemoved(Value const& value)
{
   SignalModified();
   if (removed_) {
      removed_(value);
   } else if (updated_) {
      NOT_YET_IMPLEMENTED();
   }
}

template <typename S>
void SetTrace<S>::SignalAdded(Value const& value) 
{
   SignalModified();
   if (added_) {
      added_(value);
   } else if (updated_) {
      NOT_YET_IMPLEMENTED();
   }
}

template <typename S>
void SetTrace<S>::SignalUpdated(ValueList const& added, ValueList const& removed)
{
   SignalModified();
   if (updated_) {
      updated_(added, removed);
   } else {
      for (const auto& value : added) {
         SignalAdded(value);
      }

      for (const auto& value : removed) {
         SignalRemoved(value);
      }
   }
}

#define CREATE_SET(S)  template SetTrace<S>;
#include "types/all_set_types.h"
ALL_DM_SET_TYPES

