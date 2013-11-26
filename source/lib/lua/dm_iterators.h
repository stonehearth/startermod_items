#ifndef _RADIANT_LUA_DM_ITERATORS_H
#define _RADIANT_LUA_DM_ITERATORS_H

#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

template <typename S>
class SetIterator
{
public:
   SetIterator(S const& set) { }
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_DM_ITERATORS_H