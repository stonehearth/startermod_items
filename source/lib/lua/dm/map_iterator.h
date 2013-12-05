#ifndef _RADIANT_LUA_DM_MAP_ITERATOR_H
#define _RADIANT_LUA_DM_MAP_ITERATOR_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename M>
class MapIterator
{
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
      if (i_ != map_.end()) {
         luabind::object(L, i_->first).push(L);
         luabind::object(L, i_->second).push(L);
         i_++;
         return 2;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(MapIterator)

private:
   M const& map_;
   typename M::ContainerType::const_iterator i_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_MAP_ITERATOR_H
