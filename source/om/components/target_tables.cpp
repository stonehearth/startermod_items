#include "pch.h"
#include "om/components/target_tables.ridl.h"

using namespace radiant;
using namespace radiant::om;

std::ostream& operator<<(std::ostream& os, TargetTables const& o)
{
   return (os << "[TargetTables]");
}

void TargetTables::ExtendObject(json::Node const& obj)
{
}

