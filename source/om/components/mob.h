#ifndef _RADIANT_OM_MOB_H
#define _RADIANT_OM_MOB_H

#include "math3d.h"
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/object.h"
#include "dm/store.h"
#include "namespace.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Mob : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Mob);
   static void RegisterLuaType(struct lua_State* L);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   void MoveTo(const math3d::point3& location);
   void MoveToGridAligned(const math3d::ipoint3& location);
   
   void TurnToAngle(float degrees);
   void TurnToFacePoint(const math3d::ipoint3& location);   

   math3d::aabb GetWorldAABB() const;
   math3d::aabb GetAABB() const;
   void SetAABB(const math3d::aabb& box) { aabb_.Modify() = box; }

   const math3d::point3 GetLocation() const;
   const math3d::quaternion& GetRotation() const;
   const math3d::transform& GetTransform() const;

   MobPtr GetParent() const { return (*parent_).lock(); }

   math3d::ipoint3 GetGridLocation() const;

   math3d::point3 GetWorldLocation() const;
   math3d::ipoint3 GetWorldGridLocation() const;
   math3d::transform GetWorldTransform() const;

   bool InterpolateMovement() const;
   bool IsSelectable() const;
   void SetInterpolateMovement(bool value);

   dm::Boxed<math3d::transform> const& GetBoxedTransform() const { return transform_; }

private:
   friend EntityContainer;
   void SetParent(MobPtr parent);

private:
   void TurnTo(const math3d::quaternion& orientation);

private:
   enum Flags {
      INTERPOLATE_MOVEMENT = (1 << 1),
      SELECTABLE           = (1 << 2)
   };
   void InitializeRecordFields() override;

public:
   dm::Boxed<MobRef>             parent_;
   dm::Boxed<math3d::transform>  transform_;
   dm::Boxed<math3d::aabb>       aabb_;
   dm::Boxed<int>                flags_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_MOB_H
