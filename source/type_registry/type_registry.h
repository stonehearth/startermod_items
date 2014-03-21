#ifndef _RADIANT_TYPE_REGISTRY_H
#define _RADIANT_TYPE_REGISTRY_H

/*
 * Type Registry
 *
 * Used to create the dynamic dispatch functions for all types in the system.  See
 * lib/typeinfo for more information.
 */
namespace radiant {
   class TypeRegistry {
   public:
      // Creates the type registry.  Should be called once per process.
      static void Initialize();
   };
}
#endif // _RADIANT_TYPE_REGISTRY_H
