#ifndef _RADIANT_OM_VERTICIAL_PATHING_REGION_H
#define _RADIANT_OM_VERTICIAL_PATHING_REGION_H

#include "om/om.h"
#include "om/region.h"
#include "om/object_enums.h"
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
   csg::Cube3f GetAABB() const override;

   void SetRegion(Region3BoxedPtr region) { region_ = region; }
   Region3BoxedPtrBoxed const& GetRegion() const { return region_; }

   void SetNormal(csg::Point3 const& normal) { normal_ = normal; }
   csg::Point3 GetNormal() const { return *normal_; }

private:
   void InitializeRecordFields() override;

private:
   Region3BoxedPtrBoxed    region_;
   dm::Boxed<csg::Point3>  normal_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_VERTICIAL_PATHING_REGION_H
