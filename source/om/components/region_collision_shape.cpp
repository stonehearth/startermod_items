#include "pch.h"
#include "region_collision_shape.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RegionCollisionShape::ExtendObject(json::Node const& obj)
{
   if (obj.has("region")) {
      Region3BoxedPtr region = GetStore().AllocObject<Region3Boxed>();
      region->Modify() = obj.get<csg::Region3>("region", csg::Region3());
      SetRegion(region);
   }
}
