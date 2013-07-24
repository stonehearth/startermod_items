#pragma once
#include "object.h"
#include "store.pb.h"
#include "radiant_luabind.h"
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

   class Promise
   {
   public:
      Promise(const Boxed& c, const char* reason) {
         guard_ = c.TraceValue(reason, [=](Boxed::ValueType const&) {
            for (auto& cb : changedCbs_) {
               luabind::call_function<void>(cb);
            }
         });
      }
      ~Promise() {
         LOG(WARNING) << "killing Promise...";
      }

   public:
      static void RegisterLuaType(struct lua_State* L) {
         using namespace luabind;
         module(L) [
            class_<Promise>(typeid(Promise).name())
               .def("on_changed",&Promise::PushChangedCb)
               .def("destroy",   &Promise::Destroy)
         ];
      }

   public:
      void Destroy() {
         guard_.Clear();
         changedCbs_.clear();
      }

      Promise* PushChangedCb(luabind::object cb) {
         changedCbs_.push_back(cb);
         return this;
      }

   private:
      dm::Guard                     guard_;
      std::vector<luabind::object>  changedCbs_;
   };

   Promise* CreatePromise(const char* reason) const {
      return new Promise(*this, reason);
   }

   static void RegisterLuaType(struct lua_State* L) {
      using namespace luabind;
      module(L) [
         class_<Boxed>(typeid(Boxed).name())
            .def(tostring(const_self))
            .def("trace",             &Boxed::CreatePromise)
      ];
      Promise::RegisterLuaType(L);
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

