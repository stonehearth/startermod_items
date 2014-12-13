#ifndef _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H
#define _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H

#include "radiant.h"
#include "om/namespace.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_OM_NAMESPACE

// consider refactoring this with the map_iterator_wrapper
template <typename M>
class StdMapIteratorWrapper
{
private:
   template <typename V>
   struct ValidEntry {
      bool operator()(typename M::const_iterator& i) {
         return true;
      }
   };

   template <typename V>
   struct ValidEntry<std::weak_ptr<V>> {
      bool operator()(typename M::const_iterator& i) {
         return !i->second.expired();
      }
   };

public:
   StdMapIteratorWrapper(M const& map) : map_(map), i_(map.begin())
   {
   }

   static int NextIteration(lua_State *L)
   {
      StdMapIteratorWrapper* iter = object_cast<StdMapIteratorWrapper*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      // if the current entry is invalid, find the next valid entry
      while (i_ != map_.end() && !ValidEntry<typename M::value_type>()(i_)) {
         ++i_;
      }

      if (i_ == map_.end()) {
         return 0;
      }

      luabind::object(L, i_->first).push(L);
      luabind::object(L, i_->second).push(L);

      // point to the next entry
      ++i_;
      return 2;
   }

private:
   NO_COPY_CONSTRUCTOR(StdMapIteratorWrapper)

private:
   M const&                   map_;
   typename M::const_iterator i_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LUA_STD_MAP_ITERATOR_WRAPPER_H
