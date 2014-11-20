#include "radiant.h"
#include "dm/map_util.h"
#include "static_string.h"

using namespace ::radiant;
using namespace ::radiant::core;


StaticString::StaticString(std::string const& s)
{
   _value = dm::SafeCStringTable()(s.c_str());
}
