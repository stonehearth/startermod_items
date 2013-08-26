#ifndef _RADIANT_OM_REGION_COLLSION_SHAPE_H
#define _RADIANT_OM_REGION_COLLSION_SHAPE_H

#include "om/om.h"
#include "om/region.h"
#include "om/object_enums.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "csg/region.h"
#include "collision_shape.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RegionCollisionShape : public Component,
                             public CollisionShape
{
public:
   DEFINE_OM_OBJECT_TYPE(RegionCollisionShape, region_collision_shape);

   CollisionType GetType() const override { return CollisionShape::REGION; }
   csg::Cube3f GetAABB() const override;

   BoxedRegion3Ptr GetRegionPtr() const { return *region_; }
   void SetRegionPtr(BoxedRegion3Ptr region) { region_ = region; }

public: // used only only for lua promises
   dm::Boxed<BoxedRegion3Ptr> const& GetRegionField() const { return region_; }

private:
   void InitializeRecordFields() override;

private:
   dm::Boxed<BoxedRegion3Ptr>   region_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_COLLSION_SHAPE_H
