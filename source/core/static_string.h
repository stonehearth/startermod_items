#ifndef _RADIANT_CORE_STATIC_STRING_H
#define _RADIANT_CORE_STATIC_STRING_H

#include <functional>
#include "radiant_macros.h"
#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

/*
 * -- StaticString class
 *
 * Used to create a "const char*" which can be compared with ==, regardless of the
 * location (e.g. created from a literal, a std::string, or a char[]).  Also guaranteed
 * to never be deallocated (may have to revisit that...)!
 *
 */

class StaticString
{
public:
   StaticString(StaticString const& );
   StaticString(std::string const& s);
   StaticString(const char* s);
   StaticString(const char* s, size_t len);
   operator const char*() const { return _value; }

   // for when we KNOW a const char* came from another StaticString and we don't
   // want to hash it again.
   static StaticString FromPreviousStaticStringValue(const char* value) {
      StaticString s;
      s._value = value;
      return s;
   }

   struct Hash {
      inline std::size_t operator()(StaticString const& s) {
         return std::hash<const char *>()(static_cast<const char*>(s));
      }
   };

private:
   StaticString() { }

private:
   const char*    _value;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_STATIC_STRING_H
