#ifndef _RADIANT_CORE_OBJECT_COUNTER_H
#define _RADIANT_CORE_OBJECT_COUNTER_H

#include "radiant_macros.h"
#include "namespace.h"

#include <map>
#include <unordered_map>
#include <typeindex>
#include <boost/smart_ptr/detail/spinlock.hpp>

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
   typedef std::unordered_map<std::type_index, int> CounterMap;

public:
   typedef std::function<bool(std::type_index const&, int)> ForEachObjectCountCb;
   typedef std::unordered_map<void *, std::pair<int, std::type_index>> ObjectMap;

   static CounterMap GetObjectCounts();
   static ObjectMap GetObjects();
   static void ForEachObjectDeltaCount(CounterMap const& checkpoint, ForEachObjectCountCb cb);
   static void ForEachObjectCount(ForEachObjectCountCb cb);
   static void TrackObjectLifetime(bool enable);

protected:
   static void IncrementObjectCount(void* that, std::type_info const& t);
   static void DecrementObjectCount(void* that, std::type_info const& t);
   static int GetObjectCount(std::type_info const& t);

private:
   typedef std::unordered_map<void *, std::pair<int, std::type_index>> ObjectMap;

private:
   static boost::detail::spinlock __lock;
   static CounterMap __counters;
   static ObjectMap __objects;
   static bool __track_objects;
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
