#include "radiant.h"
#include "radiant_logger.h"
#include "dm/map_util.h"
#include "static_string.h"

using namespace ::radiant;
using namespace ::radiant::core;

StaticString StaticString::Empty("");

StaticString::StaticString() :
   _value(Empty._value)
{
}

StaticString::StaticString(std::string const& s) :
   _value(ToStaticString()(s.c_str()))
{
}

StaticString::StaticString(const char* s) :
   _value(ToStaticString()(s))
{
}

StaticString::StaticString(const char* s, size_t len) :
   _value(ToStaticString()(s, len))
{   
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
   std::size_t operator()(std::string const& s) const {
      return Dbj2Hash(s.c_str());
   }
};

const char* StaticString::ToStaticString::operator()(const char* key, size_t n)
{
   static std::string __key;
   static tbb::spin_mutex __mutex;
   static std::unordered_set<std::string, StringHash> __strtab;

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
   if (n == std::string::npos) {
      n = strlen(key);
   }
   __key.replace(0, std::string::npos, key, n);

   auto i = __strtab.find(__key);
   if (i == __strtab.end()) {
      i = __strtab.insert(__key).first;
   }

   return i->c_str();
}

