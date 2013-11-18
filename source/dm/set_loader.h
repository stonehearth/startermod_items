#ifndef _RADIANT_DM_SET_LOADER_H
#define _RADIANT_DM_SET_LOADER_H

#include "dm.h"
#include "store.pb.h"
#include "protocol.h"
#include "set.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename T>
void Load(Set<T>& set, Protocol::Value const& value)
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
void SaveObject(Set<T> const& set, Protocol::Value* msg)
{
   NOT_YET_IMPLEMENTED();
}

template <typename T>
void SaveObjectDelta(std::vector<typename Set<T>::Value> const& added,
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

#endif // _RADIANT_DM_SET_LOADER_H

