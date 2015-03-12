#include "radiant.h"
#include "dm/map_util.h"
#include "static_string.h"

using namespace ::radiant;
using namespace ::radiant::core;


StaticString::StaticString(std::string const& s)
{
   _value = dm::SafeCStringTable()(s.c_str());
}

StaticString::StaticString(const char* s)
{
   _value = dm::SafeCStringTable()(s);
}

StaticString::StaticString(const char* s, size_t len)
{
   _value = dm::SafeCStringTable()(s, len);
}
