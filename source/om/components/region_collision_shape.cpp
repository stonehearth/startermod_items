#include "pch.h"
#include "om/region.h"
#include "region_collision_shape.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, RegionCollisionShape const& o)
{
   return (os << "[RegionCollisionShape]");
}

void RegionCollisionShape::ExtendObject(json::Node const& obj)
{
   if (obj.has("region")) {
      region_ = GetStore().AllocObject<Region3Boxed>();
      (*region_)->Set(obj.get("region", csg::Region3()));
   }
}
