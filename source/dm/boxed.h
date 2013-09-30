#pragma once
#include "object.h"
#include "store.pb.h"
#include "radiant_luabind.h"
#include "lua/register.h"
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

struct Integer {
   static decltype(Protocol::integer) extension;
   int value;
};

template <class T, int OT = BoxedObjectType>
class Boxed : public Object
{
public:
   enum { DmType = OT };
   ObjectType GetObjectType() const override { return OT; }

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
   const Boxed& operator=(const T& rhs) {
      value_ = rhs;
      MarkChanged();
      return *this;
   }

   const Boxed& operator=(const Boxed& rhs) {
      *this = *rhs;
      return *this;
   }

   // Allow for modifications
   T& Modify() {
      MarkChanged();
      return value_;
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
            // xxx: see bug SH-7.  this is a temporary work around
            auto callbacks = changedCbs_;
            for (auto& cb : callbacks) {
               luabind::call_function<void>(cb);
            }
         });
      }
      ~Promise() {
         LOG(WARNING) << "killing Promise...";
      }

   public:
      static luabind::scope RegisterLuaType(struct lua_State* L, std::string const& tname) {
         std::string name = tname + "Promise";
         return
            lua::RegisterTypePtr<Promise>(name.c_str())
               .def("on_changed",&Promise::PushChangedCb)
               .def("destroy",   &Promise::Destroy);
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

   std::shared_ptr<Promise> CreatePromise(const char* reason) const {
      return std::make_shared<Promise>(*this, reason);
   }

   static luabind::scope RegisterLuaType(struct lua_State* L, std::string tname = std::string()) {
      if (tname.empty()) {
         tname = lua::GetTypeName<Boxed>();
      }
      return
         lua::RegisterObject<Boxed>(tname.c_str())
            .def("get",               &Boxed::Get)
            .def("modify",            &Boxed::Modify)
            .def("trace",             &Boxed::CreatePromise)
         ,
         Promise::RegisterLuaType(L, tname);
   }

private:
   Boxed(const Boxed<T>&); // opeartor= is ok, though.

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

template <class T, int OT>
std::ostream& operator<<(std::ostream& os, const Boxed<T, OT>& o)
{
   std::ostringstream details;
   details << o.Get();

   os << "(ObjectReference " << o.GetObjectId();
   if (!details.str().empty()) {
      os << " -> " << details.str() << " ";
   }
   os << ")";
   return os;
}

END_RADIANT_DM_NAMESPACE

