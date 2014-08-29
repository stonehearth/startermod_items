#include "radiant.h"
#include "map_util.h"

using namespace radiant;
using namespace radiant::dm;

/*
 * A fast but effective hash.  Most importantly, doesn't alloc (unlike
 * std::hash<std::string>)!
 *
 * For details, see dbj2 - http://www.cse.yorku.ca/~oz/hash.html
 *
 */

std::size_t dm::Dbj2Hash(const char *str)
{
   unsigned long hash = 5381;
   int c;
   while (c = *str++) {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   }
   return hash;
}

/*
 * Hash all `key`s with the same value into a single, stable char*
 */
template <int Namespace>
const char* CStringKeyTransform<Namespace>::operator()(const char* key)
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

template <int Namespace> std::string CStringKeyTransform<Namespace>::__key;
template <int Namespace> tbb::spin_mutex CStringKeyTransform<Namespace>::__mutex;
template <int Namespace> std::unordered_set<std::string, StringHash> CStringKeyTransform<Namespace>::__strtab;

template CStringKeyTransform<-1>;
template CStringKeyTransform<0>;
