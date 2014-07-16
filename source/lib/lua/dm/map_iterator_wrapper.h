#ifndef _RADIANT_LUA_DM_MAP_ITERATOR_WRAPPER_H
#define _RADIANT_LUA_DM_MAP_ITERATOR_WRAPPER_H

#include "lib/lua/lua.h"
#include "dm/dm.h"

BEGIN_RADIANT_LUA_NAMESPACE

/*
 * -- MapIteratorWrapper class
 *
 * Exposes a dm::MapIterator to lua.  Most of the machineary in here is related to
 * changing the cpp iterator interface into the lua geneartor function one, but there
 * are some caveats.  We would like to maintain the lua guarantee that removing the
 * current element during iteration is safe.  Luckily, the M::Iterator class ensures
 * this for us.  Secondly, we want to make sure we don't dereference bad memory if
 * the results returned from the lua geneartor function are used after the map itself
 * is gc'ed.  To accomplish this, the Iterator holds onto a dm::ObjectRef, and only
 * dereferences the map (or any map iterators!) if that reference is not expired.
 * Sometimes this will be the actual map, sometimes it will be an object that owns
 * the map (e.g. a dm::Record).  Regardless, the `owner` protects access to the map.
 *
 */

template <typename M>
class MapIteratorWrapper
{
private:
   template <typename V>
   struct AdvanceIterator {
      void operator()(M const& map, typename M::Iterator& i) {
         ++i;
      }
   };

   template <typename V>
   struct AdvanceIterator<std::weak_ptr<V>> {
      void operator()(M const& map, typename M::Iterator& i) {
         do {
            ++i;
         } while (i != map.end() && i->second.expired());
      }
   };

public:
   MapIteratorWrapper(::radiant::dm::ObjectPtr owner, M const& map) : owner_(owner), map_(map), i_(map.begin())
   {
   }

   static int NextIteration(lua_State *L)
   {
      MapIteratorWrapper* iter = object_cast<MapIteratorWrapper*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      // Bail if we've reached the end of the map or the owner controlling the
      // lifetime of the map has already gone away.
      if (owner_.expired() || i_ == map_.end()) {
         return 0;
      }
      luabind::object(L, i_->first).push(L);
      luabind::object(L, i_->second).push(L);
      AdvanceIterator<typename M::Value>()(map_, i_);
      return 2;
   }

private:
   NO_COPY_CONSTRUCTOR(MapIteratorWrapper)

private:
   ::radiant::dm::ObjectRef   owner_;
   M const&                   map_;
   typename M::Iterator       i_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_MAP_ITERATOR_WRAPPER_H
