#ifndef _RADIANT_DM_RECEIVER_H
#define _RADIANT_DM_RECEIVER_H

#include "dm.h"
#include "store.pb.h"
#include "protocol.h"
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
static void Load(Set<T>& set, Protocol::Value const& value)
{
   const Protocol::Set::Update& msg = value.GetExtension(Protocol::Set::extension);
   T value;

   for (const Protocol::Value& item : msg.added()) {
      SaveImpl<T>::LoadValue(store, item, value);
      set.Insert(value);
   }
   for (const Protocol::Value& item : msg.removed()) {
      SaveImpl<T>::LoadValue(store, item, value);
      set.Remove(value);
   }
}

template <typename T>
static void Save(std::vector<typename Set<T>::Value> const& added,
                 std::vector<typename Set<T>::Value> const& removed,
                 Protocol::Value const& value)
{
   Protocol::Set::Update* msg = value->MutableExtension(Protocol::Set::extension);
   for (const T& item : added) {
      Protocol::Value* submsg = msg->add_added();
      SaveImpl<T>::SaveValue(store, submsg, item);
   }
   for (const T& item : removed) {
      Protocol::Value* submsg = msg->add_removed();
      SaveImpl<T>::SaveValue(store, submsg, item);
   }
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECEIVER_H

