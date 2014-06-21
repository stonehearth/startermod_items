#ifndef _RADIANT_LUA_DM_MAP_ITERATOR_H
#define _RADIANT_LUA_DM_MAP_ITERATOR_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename M>
class MapIterator
{
private:
   typedef typename M::ContainerType::const_iterator Iterator;

   template <typename V>
   struct AdvanceIterator {
      void operator()(M const& map, Iterator& i) {
         i++;
      }
   };

   template <typename V>
   struct AdvanceIterator<std::weak_ptr<V>> {
      void operator()(M const& map, Iterator& i) {
         do {
            i++;
         } while (i != map.end() && i->second.expired());
      }
   };

public:
   MapIterator(M const& map) : map_(map)
   {
      i_ = map_.begin();
   }

   static int NextIteration(lua_State *L)
   {
      MapIterator* iter = object_cast<MapIterator*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      /*
       * xxx: This is not safe at all.  It assumes the lua code won't remove anything
       * from the map during iteration, which is EXTREMELY unlikely.  A proper implementation
       * would re-generate the iterator from the previous key when the map is changed.
       */
      if (i_ != map_.end()) {
         luabind::object(L, i_->first).push(L);
         luabind::object(L, i_->second).push(L);
         AdvanceIterator<typename M::Value>()(map_, i_);
         return 2;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(MapIterator)

private:
   M const& map_;
   Iterator i_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_MAP_ITERATOR_H
