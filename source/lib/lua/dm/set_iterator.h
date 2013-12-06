#ifndef _RADIANT_LUA_DM_SET_ITERATOR_H
#define _RADIANT_LUA_DM_SET_ITERATOR_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename S>
class SetIterator
{
public:
   SetIterator(S const& set) : set_(set) {
      i_ = set_.begin();
   }

   static int NextIteration(lua_State *L)
   {
      SetIterator* iter = object_cast<SetIterator*>(object(from_stack(L, -2)));
      return iter->Next(L);
   }

   int Next(lua_State *L) {
      if (i_ != map_.end()) {
         luabind::object(L, *i_).push(L);
         i_++;
         return 1;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(SetIterator)

private:
   const S& set_;
   typename S::ContainerType::const_iterator i_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_SET_ITERATOR_H