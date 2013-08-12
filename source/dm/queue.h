#pragma once
#include "object.h"
#include "store.pb.h"
#include <vector>
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T>
class Queue : public Object
{
public:
   typedef T ValueType;
   typedef std::vector<T> ContainerType;

   //static decltype(Protocol::Queue::contents) extension;
   DEFINE_DM_OBJECT_TYPE(Queue);
   Queue() : Object() { }

   bool IsEmpty() {
      return items_.empty();
   }

   const T& Front() const {
      return items_.empty() ? T() : items_front();
   }

   int Size() const {
      return items_.size();
   }

   void Clear() {
      items_.clear();
      MarkChanged();
   }

   void Enqueue(const T& item) {
      items_.push_back(item);
      MarkChanged();
   }

   T Dequeue() {
      T result;
      if (!items_.empty) {
         result = items_.front();
         items_.pop_front();
         MarkChanged();
      }
      return result;
   }


   typename ContainerType::const_iterator begin() const { return items_.begin(); }
   typename ContainerType::const_iterator end() const { return items_.end(); }

public:
   void SaveValue(const Store& store, Protocol::Value* valmsg) const {
      Protocol::Queue::Update* msg = valmsg->MutableExtension(Protocol::Queue::extension);
      for (const T& item : items_) {
         Protocol::Value* submsg = msg->add_contents();
         SaveImpl<T>::SaveValue(store, submsg, item);
      }
   }
   void LoadValue(const Store& store, const Protocol::Value& valmsg) {
      const Protocol::Queue::Update& msg = valmsg.GetExtension(Protocol::Queue::extension);
      T value;

      items_.clear();
      for (const Protocol::Value& item : msg.contents()) {
         SaveImpl<T>::LoadValue(store, item, value);
         items_.push_back(value);
      }
   }

private:
   NO_COPY_CONSTRUCTOR(Queue<T>);

private:
   std::vector<T>          items_;
};

END_RADIANT_DM_NAMESPACE
