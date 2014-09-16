#ifndef _RADIANT_DM_MAP_UTIL_H_
#define _RADIANT_DM_MAP_UTIL_H_

#include "dm.h"
#include <unordered_set>
#include <tbb/spin_mutex.h>

BEGIN_RADIANT_DM_NAMESPACE

std::size_t Dbj2Hash(const char* str);

/*
 * class SharedCStringHash
 *
 * Used for dm::Maps with CStrings as the keys (see CStringKeyTransform).  Since all those
 * strings are the same pointer value anyway, we use a std::hash<void*> for performance/
 */

typedef std::hash<const void*> SharedCStringHash;

/*
 * class StringHash
 *
 * An efficient, yet effective hash for std::string.  std::hash<std::string>() has
 * been known to malloc (lame!).  Use this if that's just not your style.
 */

struct StringHash {
   std::size_t operator()(std::string const& s) {
      return Dbj2Hash(s.c_str());
   }
};

/*
 * class NopKeyTransform
 *
 * The default KeyTransform functor for dm::Maps.  Does nothing.
 */

template <typename K>
struct NopKeyTransform {
   K const& operator()(K const& key) {
      return key;
   }
};


/*
 * class CStringKeyTransform
 *
 * A KeyTransform functor for dm::Maps which hashes all keys of the
 * same value to the same pointer.  Let's us store const char*'s in
 * maps (and more importantly, look them up by const char*, which avoids
 * lots of allocations required to use a std::string).
 *
 * Namespace is used to create map-specific instances of the CStringKeyTransform
 * class, rather than using one gigantic hashtable globally.
 */

template <int Namespace=-1>
class CStringKeyTransform
{
public:
   const char* operator()(const char* key);

private:
   static std::string __key;
   static tbb::spin_mutex __mutex;
   static std::unordered_set<std::string, StringHash> __strtab;
};

// A global instance that everyone can share if you don't really care about how big
// the table gets.
//
typedef CStringKeyTransform<-1> SafeCStringTable;

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_UTIL_H_
