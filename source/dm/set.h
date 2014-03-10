#ifndef _RADIANT_DM_SET_H_
#define _RADIANT_DM_SET_H_

#include <vector>
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T>
class Set : public Object
{
public:
   typedef T Value;
   typedef std::vector<T> ContainerType;

   Set() : Object() { }
   DEFINE_DM_OBJECT_TYPE(Set, set);
   std::ostream& GetDebugDescription(std::ostream& os) const {
      return (os << "set size:" << items_.size());
   }

   void GetDbgInfo(DbgInfo &info) const override;
   void LoadValue(SerializationType r, Protocol::Value const& value) override;
   void SaveValue(SerializationType r, Protocol::Value* msg) const override;

   void Clear();
   void Remove(const T& item);
   void Add(const T& item);

   bool IsEmpty() const;
   int Size() const;
   bool Contains(const T& item);

   typename std::vector<T>::iterator begin() { return items_.begin(); }
   typename std::vector<T>::iterator end() { return items_.end(); }
   typename std::vector<T>::const_iterator begin() const { return items_.begin(); }
   typename std::vector<T>::const_iterator end() const { return items_.end(); }

public:
   ContainerType const& GetContainer() const;

private:
   NO_COPY_CONSTRUCTOR(Set<T>);

private:
   ContainerType          items_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_SET_H_
