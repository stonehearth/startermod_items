#include "pch.h"
#include "mod_list.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, ModList const& o)
{
   return (os << "[ModList]");
}

void ModList::ExtendObject(json::Node const& obj)
{
}
