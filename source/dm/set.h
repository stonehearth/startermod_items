#pragma once
#include "object.h"
#include "protocols/store.pb.h"
#include <vector>
#include "dm.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "dbg_indenter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T>
class Set : public Object
{
public:
   typedef T Value;
   typedef std::vector<T> ContainerType;

   //static decltype(Protocol::Set::contents) extension;
   DEFINE_DM_OBJECT_TYPE(Set, set);
   DECLARE_STATIC_DISPATCH(Set);
   Set() : Object() { }


   void GetDbgInfo(DbgInfo &info) const override {
      if (WriteDbgInfoHeader(info)) {
         info.os << " [" << std::endl;
         {
            Indenter indent(info.os);
            auto i = items_.begin(), end = items_.end();
            while (i != end) {
               SaveImpl<T>::GetDbgInfo(*i, info);
               if (++i != end) {
                  info.os << ",";
               }
               info.os << std::endl;
            }
         }
         info.os << "]";
      }
   }

   const std::vector<T>& GetContents() const { return items_; }

   void Clear() {
      while (!items_.empty()) {
         Remove(items_.back());
      }
   }
   void Remove(const T& item) {
      // this could be faster...
      if (stdutil::UniqueRemove(items_, item)) {
         GetStore().OnSetRemoved(*this, item);
      }
   }

   void Insert(const T& item) {
      if (!stdutil::contains(items_, item)) {
         GetStore().OnSetAdded(*this, item);
         items_.push_back(item);
      }
   }

   bool IsEmpty() const {
      return items_.empty();
   }

   int Size() const {
      return items_.size();
   }

   bool Contains(const T& item) {
      return stdutil::contains(items_, item);
   }

   typename std::vector<T>::const_iterator begin() const { return items_.begin(); }
   typename std::vector<T>::const_iterator end() const { return items_.end(); }

   std::ostream& ToString(std::ostream& os) const {
      return (os << "(Set size:" << items_.size() << ")");
   }

private:
   NO_COPY_CONSTRUCTOR(Set<T>);

private:
   std::vector<T>          items_;
};

template <typename T>
static std::ostream& operator<<(std::ostream& os, const Set<T>& in)
{
   return in.ToString(os);
}

END_RADIANT_DM_NAMESPACE
