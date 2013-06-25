#ifndef _RADIANT_OM_VERTICIAL_PATHING_REGION_H
#define _RADIANT_OM_VERTICIAL_PATHING_REGION_H

#include "math3d.h"
#include "om/om.h"
#include "om/region.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "csg/region.h"
#include "collision_shape.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class VerticalPathingRegion : public Component,
                              public CollisionShape
{
public:
   DEFINE_OM_OBJECT_TYPE(VerticalPathingRegion, vertical_pathing_region);

   CollisionType GetType() const override { return CollisionShape::REGION; }
   math3d::aabb GetAABB() const override;

   const csg::Region3& GetRegion() const { return **GetRegionPtr(); }
   RegionPtr GetRegionPtr() const { return (*region_).lock(); }
   void SetRegionPtr(RegionPtr region) { region_ = region; }

private:
   void InitializeRecordFields() override;

private:
   dm::Boxed<RegionRef>   region_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_VERTICIAL_PATHING_REGION_H
