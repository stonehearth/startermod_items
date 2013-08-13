#pragma once
#include "object.h"
#include "store.pb.h"
#include <vector>
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T, int C>
class Array : public Object
{
public:
   //static decltype(Protocol::Array::contents) extension;
   DEFINE_DM_OBJECT_TYPE(Array);
   Array() : Object() { }

   int Size() const { return C; }

   dm::Guard TraceChanges(const char* reason,
                          std::function<void(int i, const T& v)> changed) const
   {
      auto cb = [=]() {
         for (int i : changed_) {
            changed(i, items_[i]);
         }
      };
      return TraceObjectChanges(reason, cb);
   }

   const T& operator[](int i) const {
      ASSERT(i >= 0 && i < C);
      return items_[i];
   }
   
   T& operator[](int i) {
      ASSERT(i >= 0 && i < C);
      MarkChanged();
      stdutil::UniqueInsert(changed_, i);
      return items_[i];
   }

   typename const T* begin() const { return items_; }
   typename const T* end() const { return items_ + C + 1; }
   typename T* begin() { return items_; }
   typename T* end() { return items_ + C + 1; }

public:
   void SaveValue(const Store& store, Protocol::Value* valmsg) const {
      for (auto i : changed_) {
         Protocol::Array::Entry* msg = valmsg->AddExtension(Protocol::Array::extension);
         msg->set_index(i);
         SaveImpl<T>::SaveValue(store, msg->mutable_value(), items_[i]);
      }
      changed_.clear();
   }
   void LoadValue(const Store& store, const Protocol::Value& valmsg) {
      int i, c = valmsg.ExtensionSize(Protocol::Array::extension);

      changed_.clear();
      for (i = 0; i < c; i++) {
         const Protocol::Array::Entry& msg = valmsg.GetExtension(Protocol::Array::extension, i);
         int index = msg.index();
         changed_.push_back(index);
         SaveImpl<T>::LoadValue(store, msg.value(), items_[index]);
      }
   }

private:
   // No copying!
   Array(const Array<T, C>& other);
   const Array<T, C>& operator=(const Array<T, C>& rhs);

private:
   T items_[C];
   mutable std::vector<int>  changed_;
};

END_RADIANT_DM_NAMESPACE
