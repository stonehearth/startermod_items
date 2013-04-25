#pragma once
#include "object.h"
#include "store.pb.h"
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

struct Integer {
   static decltype(Protocol::integer) extension;
   int value;
};

template <class T> class Boxed : public Object
{
public:
   DEFINE_DM_OBJECT_TYPE(Boxed);

   typedef T   ValueType;

   Boxed() : Object() { }

   // Still needs to get initialized later!
   Boxed(const T& v) : Object(), value_(v) { }

   Boxed(Store& store)
   {
      Initialize(store);
   }

   Boxed(Store& store, const T& v) :
      value_(v) 
   {
      Initialize(store)
   }

   // Make a boxed value behave just like a value for read-only operations.
   const T& Get() const { return value_; }
   const T& operator*() const { return value_; }

   operator const T&() const { return value_; }
   const Boxed<T>& operator=(const T& rhs) {
      value_ = rhs;
      MarkChanged();
      return *this;
   }

   const Boxed<T>& operator=(const Boxed<T>& rhs) {
      *this = *rhs;
      return *this;
   }

   // Allow for modifications
   T& Modify() {
      MarkChanged();
      return value_;
   }
   std::ostream& Log(std::ostream& os, std::string indent) const override {
      os << "boxed [oid:" << GetObjectId() << " value:" << Format<T>(value_, indent) << "]" << std::endl;
      return os;
   }

   dm::Guard TraceValue(const char* reason, std::function<void(const T&)> fn) const {
      auto cb = [=]() {
         fn(value_);
      };
      return TraceObjectChanges(reason, cb);
   }

private:
   Boxed(const Boxed<T>&); // opeartor= is ok, though.
   void CloneObject(Object* c, CloneMapping& mapping) const override {
      Boxed<T>& copy = static_cast<Boxed<T>&>(*c);

      mapping.objects[GetObjectId()] = copy.GetObjectId();
      SaveImpl<T>::CloneValue(c->GetStore(), value_, &copy.value_, mapping);
   }

protected:
   void SaveValue(const Store& store, Protocol::Value* msg) const override {
      SaveImpl<T>::SaveValue(store, msg, value_);
   }
   void LoadValue(const Store& store, const Protocol::Value& msg) override {
      SaveImpl<T>::LoadValue(store, msg, value_);
   }

private:
   T     value_;
};


template<class T>
struct Formatter<Boxed<T>>
{
   static std::ostream& Log(std::ostream& out, const Boxed<T>& value, const std::string indent) {
      return value.Log(out, indent);
   }
};

END_RADIANT_DM_NAMESPACE

