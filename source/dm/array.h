#ifndef _RADIANT_DM_ARRAY_H_
#define _RADIANT_DM_ARRAY_H_

#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T, int C>
class Array : public Object
{
public:
   typedef T Value;

   Array() : Object() { }
   DEFINE_DM_OBJECT_TYPE(Array, array);
   std::ostream& GetDebugDescription(std::ostream& os) const {
      return (os << "array size:" << C);
   }

   void LoadValue(SerializationType r, Protocol::Value const& value) override;
   void SaveValue(SerializationType r, Protocol::Value* msg) const override;
   void GetDbgInfo(DbgInfo &info) const override;

   int Size() const { return C; }

   const T& operator[](int i) const;
   void Set(uint i, T const& value);

   typename const T* begin() const { return items_; }
   typename const T* end() const { return items_ + C + 1; }
   typename T* begin() { return items_; }
   typename T* end() { return items_ + C + 1; }

private:
   NO_COPY_CONSTRUCTOR(Array);

private:
   T items_[C];
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_ARRAY_H_
