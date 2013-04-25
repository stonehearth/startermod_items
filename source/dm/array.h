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
   void CloneObject(Object* obj, CloneMapping& mapping) const override {
      Array<T, C>& copy = static_cast<Array<T, C>&>(*obj);

      mapping.objects[GetObjectId()] = copy.GetObjectId();
      for (int i = 0; i < C; i++) {
         SaveImpl<T>::CloneValue(obj->GetStore(), items_[i], &copy[i], mapping);
      }
   }
   std::ostream& Log(std::ostream& os, std::string indent) const override {
      os << "array [oid:" << GetObjectId() << "] {" << std::endl;
      std::string i2 = indent + std::string("  ");
      for (int i = 0; i < C; i++) {
         os << i2 << i << " : " << Format<T>(items_[i], i2) << std::endl;
      }
      os << indent << "}" << std::endl;
      return os;
   }

private:
   // No copying!
   Array(const Array<T, C>& other);
   const Array<T, C>& operator=(const Array<T, C>& rhs);

private:
   T items_[C];
   mutable std::vector<int>  changed_;
};

#if 0
template<class T, int C>
struct SaveImpl<Array<T, C>>
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const Array<T, C>& obj) {
      obj.SaveValue(store, msg);
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, Array<T, C>& obj) {
      obj.LoadValue(store, msg);
   }
   static void CloneValue(Store& store, Array<T, C>& obj, Object& copy, CloneMapping& mapping) {
      obj.CloneValue(static_cast<Array<T, C>&>(copy), mapping);
   }
};
#endif

END_RADIANT_DM_NAMESPACE
