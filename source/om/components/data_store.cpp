#include "pch.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, DataStore const& o)
{
   return (os << "[DataStore]");
}
