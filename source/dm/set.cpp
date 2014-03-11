#include "radiant.h"
#include "radiant_stdutil.h"
#include "set.h"
#include "set_trace.h"
#include "store.h"
#include "dbg_info.h"
#include "dbg_indenter.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <class V>
std::shared_ptr<SetTrace<Set<V>>> Set<V>::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceSetChanges(reason, *this, category);
}

template <class V>
TracePtr Set<V>::TraceObjectChanges(const char* reason, int category) const
{
   return GetStore().TraceSetChanges(reason, *this, category);
}

template <class V>
TracePtr Set<V>::TraceObjectChanges(const char* reason, Tracer* tracer) const
{
   return GetStore().TraceSetChanges(reason, *this, tracer);
}

template <class T>
void Set<T>::LoadValue(SerializationType r, Protocol::Value const& value)
{
   auto const& msg = value.GetExtension(Protocol::Set::extension);
   T v;

   for (const Protocol::Value& item : msg.added()) {
      SaveImpl<T>::LoadValue(GetStore(), r, item, v);
      Add(v);
   }
   for (const Protocol::Value& item : msg.removed()) {
      SaveImpl<T>::LoadValue(GetStore(), r, item, v);
      Remove(v);
   }
}

template <class T>
void Set<T>::SaveValue(SerializationType r, Protocol::Value* value) const
{
   Store const& store = GetStore();
   Protocol::Set::Update* msg = value->MutableExtension(Protocol::Set::extension);

   for (const Value& item : items_) {
      Protocol::Value* submsg = msg->add_added();
      SaveImpl<Value>::SaveValue(store, r, submsg, item);
   }
}

template <class T>
void Set<T>::GetDbgInfo(DbgInfo &info) const {
   if (WriteDbgInfoHeader(info)) {
      info.os << " [" << std::endl;
      {
         Indenter indent(info.os);
         auto i = items_.begin(), end = items_.end();
         while (i != end) {
            SaveImpl<T>::GetDbgInfo(*i, info);
            if (++i != end) {
               info.os << ",";
            }
            info.os << std::endl;
         }
      }
      info.os << "]";
   }
}

template <class T>
void Set<T>::Clear() {
   while (!items_.empty()) {
      Remove(items_.back());
   }
}

template <class T>
void Set<T>::Remove(const T& item) {
   // this could be faster...
   if (stdutil::UniqueRemove(items_, item)) {
      GetStore().OnSetRemoved(*this, item);
   }
}

template <class T>
void Set<T>::Add(const T& item) {
   if (!stdutil::contains(items_, item)) {
      GetStore().OnSetAdded(*this, item);
      items_.push_back(item);
   }
}

template <class T>
bool Set<T>::IsEmpty() const {
   return items_.empty();
}

template <class T>
int Set<T>::Size() const {
   return items_.size();
}

template <class T>
bool Set<T>::Contains(const T& item) {
   return stdutil::contains(items_, item);
}

template <class T>
typename Set<T>::ContainerType const& Set<T>::GetContainer() const
{
   return items_;
}

#define CREATE_SET(S)      template S;
#include "types/all_loader_types.h"
#include "types/all_set_types.h"
ALL_DM_SET_TYPES
