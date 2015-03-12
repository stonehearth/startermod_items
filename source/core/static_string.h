#ifndef _RADIANT_CORE_STATIC_STRING_H
#define _RADIANT_CORE_STATIC_STRING_H

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
   StaticString(std::string const& s);
   StaticString(const char* s);
   StaticString(const char* s, size_t len);
   operator const char*() const { return _value; }

private:
   const char*    _value;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_STATIC_STRING_H
