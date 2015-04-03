#include "radiant.h"
#include "dm/map_util.h"
#include "static_string.h"

using namespace ::radiant;
using namespace ::radiant::core;

StaticString::StaticString() :
   _value(Empty._value)
{
}

StaticString::StaticString(std::string const& s)
{
   _value = ToStaticString()(s.c_str());
}

StaticString::StaticString(const char* s)
{
   _value = ToStaticString()(s);
}


/*
 * A fast but effective hash.  Most importantly, doesn't alloc (unlike
 * std::hash<std::string>)!
 *
 * For details, see dbj2 - http://www.cse.yorku.ca/~oz/hash.html
 *
 */

std::size_t Dbj2Hash(const char *str)
{
   unsigned long hash = 5381;
   int c;
   while (c = *str++) {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   }
   return hash;
}

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

std::string __key;
tbb::spin_mutex __mutex;
std::unordered_set<std::string, StringHash> __strtab;
StaticString StaticString::Empty("");

const char* StaticString::ToStaticString::operator()(const char* key)
{
   /*
    * The same types get created on the client and the server, so we need to
    * grab a lock when modifying the table.  You could imagine using a reader/writer
    * lock here, but the whole operation is so blindingly fast a spinlock is
    * probably best.
    */
   tbb::spin_mutex::scoped_lock lock(__mutex);

   /*
    * Use the static __key to do all our operations.  Constructing a new
    * string here on the stack would alloc, which is what we're trying to avoid!
    */
   __key = key;

   auto i = __strtab.find(__key);
   if (i == __strtab.end()) {
      i = __strtab.insert(__key).first;
   }

   return i->c_str();
}

