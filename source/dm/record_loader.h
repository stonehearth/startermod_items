#ifndef _RADIANT_DM_RECEIVER_H
#define _RADIANT_DM_RECEIVER_H

#include "dm.h"
#include "store.pb.h"
#include "protocol.h"
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T, typename OT>
static void Load(Boxed<T, OT>& boxed, Protocol::Value const& msg)
{
   SaveImpl<T>::LoadValue(store, msg, value_);
}

template <typename T, typename OT>
static void Save(Boxed<T, OT> const& boxed, Protocol::Value const& msg)
{
   SaveImpl<T>::SaveValue(store, msg, *boxed);
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECEIVER_H

