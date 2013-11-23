#include "radiant.h"
#include "store.h"
#include "object_trace.h"

using namespace radiant;
using namespace radiant::dm;

template <typename T>
ObjectTrace<T>::ObjectTrace(const char* reason, Object const& o, Store const& store) :
   TraceImpl(reason, o, store)
{
}

template <typename T>
std::shared_ptr<ObjectTrace<T>> ObjectTrace<T>::PushObjectState()
{
   GetStore().PushObjectState<T>(*this, GetObjectId());
   return shared_from_this();
}

template <typename T>
void ObjectTrace<T>::NotifyObjectState()
{
   SignalModified();
}

template ObjectTrace<Object>;
