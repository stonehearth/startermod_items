#pragma once
#include "object.h"
#include "store.pb.h"
#include <unordered_map>
#include "dm.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "dbg_indenter.h"
#include "map_loader.h"

BEGIN_RADIANT_DM_NAMESPACE

// TODO: make sure only non-Object types get used in containers...
template <class K, class V, class Hash=std::unordered_map<K, V>::hasher>
class Map : public Object
{
public:
   DEFINE_DM_OBJECT_TYPE(Map, map);
   DECLARE_STATIC_DISPATCH(Map);

   typedef K Key;
   typedef V Value;
   typedef std::pair<K, V> Entry;

   Map() : Object() { }

   void GetDbgInfo(DbgInfo &info) const override {
      if (WriteDbgInfoHeader(info)) {
         info.os << " {" << std::endl;
         {
            Indenter indent(info.os);
            auto i = items_.begin(), end = items_.end();
            while (i != end) {
               SaveImpl<K>::GetDbgInfo(i->first, info);
               info.os << " : ";
               SaveImpl<V>::GetDbgInfo(i->second, info);
               if (++i != end) {
                  info.os << ",";
               }
               info.os << std::endl;
            }
         }
         info.os << "}";
      }
   }

   typedef std::unordered_map<K, V, Hash> ContainerType;

   int GetSize() const { return items_.size(); }
   bool IsEmpty() const { return items_.empty(); }

   const ContainerType& GetContents() const { return items_; }

   typename ContainerType::const_iterator begin() const { return items_.begin(); }
   typename ContainerType::const_iterator end() const { return items_.end(); }
   typename ContainerType::const_iterator find(const K& key) const { return items_.find(key); }

   typename ContainerType::const_iterator RemoveIterator(typename ContainerType::const_iterator i) {      
      return items_.erase(i);
      GetStore().OnMapRemoved(*this, i->first);
   }

   void Insert(K const& key, V const& value) {
      auto result = items_.insert(std::make_pair(key, value));
      if (result.second || result.first->second != value) {
         GetStore().OnMapChanged(*this, key, value);
      }
   }
   
   void Remove(const K& key) {
      auto i = items_.find(key);
      if (i != items_.end()) {
         RemoveIterator(i);
      }
   }

   bool Contains(const K& key) const {
      return items_.find(key) != items_.end();
   }

   V Lookup(const K& key, V default) const {
      auto i = items_.find(key);
      return i != items_.end() ? i->second : default;
   }

   void Clear() {
      while (!items_.empty()) {
         Remove(items_.begin()->first);
      }
   }

   std::ostream& ToString(std::ostream& os) const {
      return (os << "(Map size:" << items_.size() << ")");
   }

private:
   NO_COPY_CONSTRUCTOR(Map);

private:
   ContainerType           items_;
};

template <typename K, typename V, typename H>
static std::ostream& operator<<(std::ostream& os, const Map<K, V, H>& in)
{
   return in.ToString(os);
}

END_RADIANT_DM_NAMESPACE
