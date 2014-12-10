#ifndef _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H
#define _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H

#include "radiant.h"
#include "om/namespace.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_OM_NAMESPACE

template <typename T>
class StdMapIteratorWrapper
{
private:
   template <typename V>
   struct AdvanceIterator {
      void operator()(T const& map, typename T::const_iterator& i) {
         ++i;
      }
   };

   template <typename V>
   struct AdvanceIterator<std::weak_ptr<V>> {
      void operator()(T const& map, typename T::const_iterator& i) {
         do {
            ++i;
         } while (i != map.end() && i->second.expired());
      }
   };

public:
   StdMapIteratorWrapper(T const& map) : map_(map), i_(map.begin())
   {
   }

   static int NextIteration(lua_State *L)
   {
      StdMapIteratorWrapper* iter = object_cast<StdMapIteratorWrapper*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      if (i_ == map_.end()) {
         return 0;
      }
      luabind::object(L, i_->first).push(L);
      luabind::object(L, i_->second).push(L);
      AdvanceIterator<typename T::value_type>()(map_, i_);
      return 2;
   }

private:
   NO_COPY_CONSTRUCTOR(StdMapIteratorWrapper)

private:
   T const&                   map_;
   typename T::const_iterator i_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H
