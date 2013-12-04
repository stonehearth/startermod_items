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

   void NextIteration(lua_State *L, luabind::object s, luabind::object var) {
      if (i_ != set_.end()) {
         luabind::object(L, *i_).push(L);
         i_++;
      } else {
         lua_pushnil(L);
      }
   }

private:
   NO_COPY_CONSTRUCTOR(SetIterator)

private:
   const S& set_;
   typename S::ContainerType::const_iterator i_;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_SET_ITERATOR_H