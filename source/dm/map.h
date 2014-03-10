#ifndef _RADIANT_DM_MAP_H_
#define _RADIANT_DM_MAP_H_

#include "object.h"
#include <unordered_map>

BEGIN_RADIANT_DM_NAMESPACE

template <class K, class V, class Hash=std::unordered_map<K, V>::hasher>
class Map : public Object
{
public:
   typedef K Key;
   typedef V Value;
   typedef std::pair<K, V> Entry;
   typedef std::unordered_map<K, V, Hash> ContainerType;

   Map() : Object() { }
   DEFINE_DM_OBJECT_TYPE(Map, map);

   void LoadValue(SerializationType r, Protocol::Value const& value) override;
   void SaveValue(SerializationType r, Protocol::Value* msg) const override;
   void GetDbgInfo(DbgInfo &info) const override;
   std::ostream& GetDebugDescription(std::ostream& os) const {
      return (os << "map size:" << items_.size());
   }

   int GetSize() const;
   bool IsEmpty() const;

   const ContainerType& GetContents() const;

   typename ContainerType::const_iterator begin() const { return items_.begin(); }
   typename ContainerType::const_iterator end() const { return items_.end(); }
   typename ContainerType::const_iterator find(const K& key) const { return items_.find(key); }

   typename ContainerType::const_iterator Remove(typename ContainerType::const_iterator i);

   void Add(K const& key, V const& value);
   void Remove(const K& key);
   bool Contains(const K& key) const;
   V Get(const K& key, V default) const;
   int Size() const { return items_.size(); }
   void Clear();

public:
   ContainerType const& GetContainer() const;

private:
   NO_COPY_CONSTRUCTOR(Map);

private:
   ContainerType           items_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_H_
