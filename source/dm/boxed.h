#ifndef _RADIANT_DM_BOXED_H_
#define _RADIANT_DM_BOXED_H_

#include <typeinfo>
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T, int OT = BoxedObjectType>
class Boxed : public Object
{
public:
   typedef T   Value;

   enum { DmType = OT };
   Boxed() : Object() { }
   static ObjectType GetObjectTypeStatic() { return OT; }
   ObjectType GetObjectType() const override { return OT; }
   const char *GetObjectClassNameLower() const override { return "boxed"; }
   std::shared_ptr<BoxedTrace<Boxed>> TraceChanges(const char* reason, int category) const;
   TracePtr TraceObjectChanges(const char* reason, int category) const override;
   TracePtr TraceObjectChanges(const char* reason, Tracer* tracer) const override;

   std::ostream& GetDebugDescription(std::ostream& os) const {
      return (os << "boxed " << typeid(T).name());
   }

   void LoadValue(SerializationType r, Protocol::Value const& msg) override;
   void SaveValue(SerializationType r, Protocol::Value* msg) const override;   
   void GetDbgInfo(DbgInfo &info) const override;

   // Make a boxed value behave just like a value for read-only operations.
   const T& Get() const { return value_; }
   const T& operator*() const { return value_; }
   operator const T&() const { return value_; }

   void Set(T const& value);
   const Boxed& operator=(const T& rhs);
   void Modify(std::function<void(T& value)> cb);

private:
   Boxed(Boxed<T, OT> const&); // opeartor= is ok, though.

private:
   T     value_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_H_

