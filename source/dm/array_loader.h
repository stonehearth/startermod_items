#ifndef _RADIANT_DM_RECEIVER_H
#define _RADIANT_DM_RECEIVER_H

#include "dm.h"
#include "store.pb.h"
#include "protocol.h"
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T, int C>
static void Load(Array<T, C>& a, Protocol::Value const& value)
{
   int i, c = valmsg.ExtensionSize(Protocol::Array::extension);
   T item;
   for (i = 0; i < c; i++) {
      const Protocol::Array::Entry& msg = valmsg.GetExtension(Protocol::Array::extension, i);
      SaveImpl<T>::LoadValue(store, msg.value(), item);
      a.Set(msg.index(), item);
   }
}

template <typename T, int C>
static void Save(std::unordered_map<uint, typename Array<T, C>::Value> const& changed
                 Protocol::Value const& value)
{
   for (const auto& entry : changed) {
      Protocol::Array::Entry* msg = valmsg->AddExtension(Protocol::Array::extension);
      msg->set_index(entry.first);
      SaveImpl<T>::SaveValue(store, msg->mutable_value(), entry.second);
   }
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECEIVER_H

