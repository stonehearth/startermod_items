#ifndef _RADIANT_CORE_OBJECT_COUNTER_H
#define _RADIANT_CORE_OBJECT_COUNTER_H

#include "radiant_macros.h"
#include "namespace.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/timer.h"

#include <map>
#include <unordered_map>
#include <typeindex>
#include <tbb/spin_mutex.h>

/* 
 * Object Counter
 *
 * Infrastructure to count the number of live objects in the world.  To use
 *
 *  1) Derive your class from ObjectCounter.  Multiple inheritance in this case
 *     is fine if you're already part of a class hierarchy.   
 *
 *  2) Define ENABLE_OBJECT_COUNTER when building.  Counting objects is very
 *     expensive (grabs a lock at every object allocation/deallocation), so only
 *     define this for debugging purposes!
 *
 *  For Example:
 *   
 *     class Foo : public ObjectCounter<Foo>
 *                 public FooBase {
 *        public:
 *            virtual ~Foo() { }
 *     };
 *
 *     int c = Foo::GetObjectCount(); // How many Foo's are there?
 */

BEGIN_RADIANT_CORE_NAMESPACE

#if !defined(ENABLE_OBJECT_COUNTER)

template <typename T> class ObjectCounter {}; // The nop implementation

#else


/*
 * -- Class ObjectCounterBase
 *
 * Hides the implementation of the object counting from the header file.  This
 * is to speed up compile times (especially when changing the implementation of
 * object counting!) and to (perhaps?) reduce the size of the image.
 */

class ObjectCounterBase
{
public:
   ObjectCounterBase() {}
   virtual ~ObjectCounterBase() {}

public:
   struct AllocationMap {
      AllocationMap(std::type_info const& t) : typeName(t.name()) { }

      std::string typeName;
      std::unordered_map<ObjectCounterBase *, perfmon::CounterValueType> allocs;
   };
   typedef std::unordered_map<std::type_index, AllocationMap> ObjectMap;

   static ObjectMap const& GetObjects();
   static void TrackObjectLifetime(bool enable);

protected:
   static void IncrementObjectCount(ObjectCounterBase* that, std::type_info const& t);
   static void DecrementObjectCount(ObjectCounterBase* that, std::type_info const& t);

private:
   static tbb::spin_mutex __lock;
   static ObjectMap __objects;
};

/*
 * -- Class ObjectCounter<T>
 *
 * Template class to count objects.  To use it, simply inherit you class
 * from ObjectCounter, passing your class type name to as the template parameter.
 * You will also need to declare you destructor as virtual.
 */
template <typename T>
class ObjectCounter : public ObjectCounterBase
{
public:
   ObjectCounter() {
      IncrementObjectCount(this, typeid(T));
   }

   virtual ~ObjectCounter() {
      DecrementObjectCount(this, typeid(T));
   }

   static int GetObjectCount() {
      return GetObjectCount(typeid(T));
   }
};
#endif

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_OBJECT_COUNTER_H
