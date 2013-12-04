#include "pch.h"
#include "vertical_pathing_region.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, VerticalPathingRegion const& o)
{
   return (os << "[VerticalPathingRegion]");
}

void VerticalPathingRegion::ExtendObject(json::Node const& obj)
{
}
