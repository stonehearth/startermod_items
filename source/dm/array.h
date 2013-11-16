#pragma once
#include "object.h"
#include "store.pb.h"
#include <vector>
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T, int C>
class Array : public Object
{
public:
   //static decltype(Protocol::Array::contents) extension;
   DEFINE_DM_OBJECT_TYPE(Array, array);
   IMPLEMENT_DYNAMIC_TO_STATIC_DISPATCH(Array);

   Array() : Object() { }

   void GetDbgInfo(DbgInfo &info) const override {
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

   int Size() const { return C; }

   const T& operator[](int i) const {
      ASSERT(i >= 0 && i < C);
      return items_[i];
   }
   
   T& operator[](int i) {
      ASSERT(i >= 0 && i < C);
      NOT_YET_IMPLEMENTED(); // can't notify that the object has changed until the client is done with it! (UG!)
      GetStore().OnArrayChanged(*this, i, items_[i]);
      return items_[i];
   }

   typename const T* begin() const { return items_; }
   typename const T* end() const { return items_ + C + 1; }
   typename T* begin() { return items_; }
   typename T* end() { return items_ + C + 1; }

private:
   // No copying!
   Array(const Array<T, C>& other);
   const Array<T, C>& operator=(const Array<T, C>& rhs);

private:
   T items_[C];
};

END_RADIANT_DM_NAMESPACE
