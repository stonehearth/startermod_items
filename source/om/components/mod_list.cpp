#include "pch.h"
#include "lib/lua/bind.h"
#include "mod_list.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, ModList const& o)
{
   return (os << "[ModList]");
}

void ModList::LoadFromJson(json::Node const& obj)
{
}

void ModList::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}
