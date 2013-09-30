#ifndef _RADIANT_CORE_UNIQUE_RESOURCE_H
#define _RADIANT_CORE_UNIQUE_RESOURCE_H

#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

/*
 * Unique resources automatically release a resource when the variable is destroyed
 * For example
 *
 *  {
 *     SOCKET s = connect(...)
 *     core::UniqueResource<SOCKET, close_socket> a(s);  // Hold onto the socket...
 *     ... lots of stuff ...
 *  }
 *  // closesocket(s) gets called down here, when a's out of scope..
 *
 * This is useful for automatically reclaiming resources when the owner of the
 * resource is destroyed.  For more info, see std::unique_ptr's documentation.
 */


/*
 * The UniqueResourceDeleter wraps the ReclaimFn in a void(*)(void) so we can
 * use a std::unique_ptr<> for most of the implementation
 */ 

template <typename Resource, void (*ReclaimFn)(Resource)> 
struct UniqueResourceDeleter
{
   void operator()(void *res) const {
      ReclaimFn(reinterpret_cast<Resource>(res));
   }
};

template <typename Resource, void (*ReclaimFn)(Resource)>
class UniqueResource : public std::unique_ptr<void, UniqueResourceDeleter<Resource, ReclaimFn>> {
public:
   typedef UniqueResourceDeleter<Resource, ReclaimFn> Deleter;
   typedef std::unique_ptr<void, UniqueResourceDeleter<Resource, ReclaimFn>> SuperClass;

   UniqueResource() : std::unique_ptr<void, Deleter>() { }
   UniqueResource(Resource res) : std::unique_ptr<void, Deleter>(reinterpret_cast<void*>(res)) { }
   UniqueResource(UniqueResource& other) { *this = other; }

   Resource get() {
      return reinterpret_cast<Resource>(SuperClass::get());
   }

   Resource const get() const {
      return reinterpret_cast<Resource>(SuperClass::get());
   }

   Resource release() {
      return reinterpret_cast<Resource>(SuperClass::release());
   }

   void reset(Resource const res) {
      SuperClass::reset(reinterpret_cast<void*>(res));
   }

   UniqueResource const& operator=(UniqueResource& other) {      
      reset(other.release());    // transfer ownership.
      return *this;
   }

private:
   /*
    * Disallow this assigment, as it leads to confusing code.  The reader needs to
    * know the type of the lhs of the operand to use this correctly, which is too
    * large of a burden.  Use "a = UniqueResource(res)" if you really want to use =.
    */
   UniqueResource const& operator=(Resource res) {
      reset(res);
      return *this;
   }
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_UNIQUE_RESOURCE_H
