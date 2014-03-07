#include "pch.h"
#include "vertical_pathing_region.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, VerticalPathingRegion const& o)
{
   return (os << "[VerticalPathingRegion]");
}

void VerticalPathingRegion::LoadFromJson(json::Node const& obj)
{
}

void VerticalPathingRegion::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }
   node.set("normal", GetNormal());
}
