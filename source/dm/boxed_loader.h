#ifndef _RADIANT_DM_BOXED_LOADER_H
#define _RADIANT_DM_BOXED_LOADER_H

#include "dm.h"
#include "protocols/store.pb.h"
#include "protocol.h"
#include "boxed.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T, int OT>
void Load(Boxed<T, OT>& boxed, Protocol::Value const& msg)
{
   SaveImpl<T>::LoadValue(store, msg, value_);
}

template <typename T, int OT>
void SaveObject(Boxed<T, OT> const& set, Protocol::Value* msg)
{
   NOT_YET_IMPLEMENTED();
}

template <typename T, int OT>
void SaveObjectDelta(typename Boxed<T, OT>::Value const& value, Protocol::Value const& msg)
{
   SaveImpl<T>::SaveValue(store, msg, value);
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_LOADER_H

