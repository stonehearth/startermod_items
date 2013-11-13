#include "pch.h"
#include "region_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RegionCollisionShape::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void RegionCollisionShape::ExtendObject(json::Node const& obj)
{
   LOG(WARNING) << "Expanding mob " << obj.write_formatted();

   if (obj.has("region")) {
      Region3BoxedPtr region = GetStore().AllocObject<Region3Boxed>();
      region->Modify() = obj.get<csg::Region3>("region", csg::Region3());
      SetRegion(region);
   }
}

csg::Cube3f RegionCollisionShape::GetAABB() const
{
   ASSERT(false);
   return csg::Cube3f();
}


RegionCollisionShape& RegionCollisionShape::SetRegion(Region3BoxedPtr r)
{
   region_ = r;
   return *this;
}
