#include "radiant.h"
#include "array.h"
#include "store.h"
#include "dbg_indenter.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;


template <class T, int C>
std::shared_ptr<ArrayTrace<Array<T, C>>> Array<T, C>::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceArrayChanges(reason, *this, category);
}


template <class T, int C>
TracePtr Array<T, C>::TraceObjectChanges(const char* reason, int category) const
{
   return GetStore().TraceArrayChanges(reason, *this, category);
}

template <class T, int C>
TracePtr Array<T, C>::TraceObjectChanges(const char* reason, Tracer* tracer) const
{
   return GetStore().TraceArrayChanges(reason, *this, tracer);
}

template <class T, int C>
void Array<T, C>::LoadValue(SerializationType r, Protocol::Value const& value)
{
   int i, c = valmsg.ExtensionSize(Protocol::Array<T, C>::extension);
   T item;
   for (i = 0; i < c; i++) {
      const Protocol::Array<T, C>::Entry& msg = valmsg.GetExtension(Protocol::Array<T, C>::extension, i);
      SaveImpl<T>::LoadValue(GetStore(), r, msg.value(), item);
      Set(msg.index(), item);
   }
}

template <class T, int C>
void Array<T, C>::SaveValue(SerializationType r, Protocol::Value* msg) const
{
   NOT_YET_IMPLEMENTED();
}

template <class T, int C>
void Array<T, C>::GetDbgInfo(DbgInfo &info) const {
   if (WriteDbgInfoHeader(info)) {
      info.os << " [" << std::endl;
      {
         Indenter indent(info.os);
         for (int i = 0; i < C; i++) {
            SaveImpl<T>::GetDbgInfo(items_[i], info);
            if (i < (C - 1)) {
               info.os << ",";
            }
            info.os << std::endl;
         }
      }
      info.os << "]";
   }
}

template <class T, int C>
const T& Array<T, C>::operator[](int i) const {
   ASSERT(i >= 0 && i < C);
   return items_[i];
}
   
template <class T, int C>
void Array<T, C>::Set(uint i, T const& value) {
   items_[i] = value;
   GetStore().OnArrayChanged(*this, i, items_[i]);
}
