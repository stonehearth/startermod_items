#ifndef _RADIANT_LIB_LUA_BIND_H
#define _RADIANT_LIB_LUA_BIND_H

#include <memory>
#include "lib/lua/lua.h"

namespace luabind { namespace detail { namespace has_get_pointer_ {

   template <class T>
   T* get_pointer(std::shared_ptr<T> const& p);

   template <class T>
   T* get_pointer(std::weak_ptr<T> const& p);

} } }

namespace boost {
   template <class T>
   T* get_pointer(std::weak_ptr<T> const& p)
   {
      auto ptr = p.lock();
      if (!ptr) {
         return nullptr;
      }
      return ptr.get();
   }
} // namespace boost

#include <luabind/luabind.hpp>
#include <luabind/scope.hpp>
#include <luabind/operator.hpp>
#include <luabind/class_info.hpp>
#include <luabind/tag_function.hpp>
#include <luabind/iterator_policy.hpp>
#include <luabind/dependency_policy.hpp>
extern "C" {
    #include "lauxlib.h"
    #include "lualib.h"
}

#endif // _RADIANT_STDUTIL_H
