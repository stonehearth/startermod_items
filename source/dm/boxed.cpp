#include "radiant.h"
#include "store.h"
#include "boxed.h"
#include "dbg_indenter.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <class T, int OT>
std::shared_ptr<BoxedTrace<Boxed<T, OT>>> Boxed<T, OT>::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceBoxedChanges(reason, *this, category);
}

template <class T, int OT>
void Boxed<T, OT>::LoadValue(Protocol::Value const& msg)
{
   SaveImpl<T>::LoadValue(GetStore(), msg, value_);
}

template <class T, int OT>
void Boxed<T, OT>::SaveValue(Protocol::Value* msg) const
{
   NOT_YET_IMPLEMENTED();
}

template <class T, int OT>
void Boxed<T, OT>::GetDbgInfo(DbgInfo &info) const {
   if (WriteDbgInfoHeader(info)) {
      info.os << " -> ";
      SaveImpl<T>::GetDbgInfo(value_, info);
   }
}

template <class T, int OT>
void Boxed<T, OT>::Set(T const& value) {
   value_ = value;
   GetStore().OnBoxedChanged(*this, value_);
}

template <class T, int OT>
const Boxed<T, OT>& Boxed<T, OT>::operator=(const T& rhs) {
   Set(rhs);
   return *this;
}

template <class T, int OT>
void Boxed<T, OT>::Modify(std::function<void(T& value)> cb) {
   cb(value_);
   GetStore().OnBoxedChanged(*this, value_);
}

#define DM_BOXED(v) template Boxed<v>;
#include "defs/boxes.h"

