#ifndef _RADIANT_OM_GRID_COLLSION_SHAPE_H
#define _RADIANT_OM_GRID_COLLSION_SHAPE_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "collision_shape.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class GridCollisionShape : public Component,
                           public CollisionShape
{
public:
   DEFINE_OM_OBJECT_TYPE(GridCollisionShape);

   virtual ~GridCollisionShape() { }

   CollisionType GetType() const override { return CollisionShape::GRID; }

   math3d::aabb GetAABB() const override;

   om::GridPtr GetGrid() const { return (*grid_).lock(); }
   void SetGrid(GridPtr grid) { grid_ = grid; }

private:
   void InitializeRecordFields() override;

private:
   dm::Boxed<GridRef>     grid_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_GRID_COLLSION_SHAPE_H
