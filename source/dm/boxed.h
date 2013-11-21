#pragma once
#include "object.h"
#include "protocols/store.pb.h"
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

   template <class T, int OT = BoxedObjectType>
class Boxed : public Object
{
public:
   enum { DmType = OT };
   ObjectType GetObjectType() const override { return OT; }
   const char *GetObjectClassNameLower() const override { return "boxed"; }

   DECLARE_STATIC_DISPATCH(Boxed);

   typedef T   Value;

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

   void GetDbgInfo(DbgInfo &info) const override {
      if (WriteDbgInfoHeader(info)) {
         info.os << " -> ";
         SaveImpl<T>::GetDbgInfo(value_, info);
      }
   }

   // Make a boxed value behave just like a value for read-only operations.
   const T& Get() const { return value_; }
   const T& operator*() const { return value_; }
   operator const T&() const { return value_; }

   void Set(T const& value) {
      value_ = value;
      GetStore().OnBoxedChanged(*this, value_);
   }

   const Boxed& operator=(const T& rhs) {
      Set(rhs);
      return *this;
   }

#if 0
   // xxx: this makes my head hurt... can we nuke it?  -- tony
   const Boxed& operator=(const Boxed& rhs) {
      *this = *rhs;
      return *this;
   }
#endif

   void Modify(std::function<void(T& value)> cb) {
      cb(value_);
      GetStore().OnBoxedChanged(*this, value_);
   }

private:
   Boxed(const Boxed<T>&); // opeartor= is ok, though.

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

