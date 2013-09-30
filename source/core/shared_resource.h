#ifndef _RADIANT_CORE_SHARED_RESOURCE_H
#define _RADIANT_CORE_SHARED_RESOURCE_H

#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

/*
 * Shared resources protect a resource for as long as someone has a reference
 * to it.  For example
 *
 *  {
 *     SOCKET s = connect(...)
 *     core::SharedResource<SOCKET, close_socket> a(s);  // Hold onto the socket...
 *     ... lots of stuff ...
 *  }
 *  // closesocket(s) gets called down here, when a's out of scope..
 *
 * This useful when you have some funky resource type shared and should be
 * reclaimed when the last reference goes away.  For more information see the
 * documentation for std::shared_ptr().
 *
 * This works for all types which can by cast to a (void *).
 *
 */

template <typename Resource, void (*ReclaimFn)(Resource)>
class SharedResource : public std::shared_ptr<void> {
public:
   typedef std::shared_ptr<void> SuperClass;
   static void FreeFn(void *res) { ReclaimFn(reinterpret_cast<Resource>(res)); }

   SharedResource() : std::shared_ptr<void>() { }
   SharedResource(Resource res) : std::shared_ptr<void>(reinterpret_cast<void*>(res), FreeFn) { }

   Resource get() {
      return reinterpret_cast<Resource>(SuperClass::get());
   }

   Resource const get() const {
      return reinterpret_cast<Resource>(SuperClass::get());
   }

};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_SHARED_RESOURCE_H
