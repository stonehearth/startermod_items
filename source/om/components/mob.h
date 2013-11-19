#ifndef _RADIANT_OM_MOB_H
#define _RADIANT_OM_MOB_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/object.h"
#include "dm/store.h"
#include "namespace.h"
#include "component.h"
#include "csg/cube.h"
#include "csg/transform.h"

BEGIN_RADIANT_OM_NAMESPACE

class Mob : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Mob, mob);
   void ExtendObject(json::Node const& obj) override;
   virtual void Describe(std::ostringstream& os) const;

   void MoveTo(const csg::Point3f& location);
   void MoveToGridAligned(const csg::Point3& location);
   
   void TurnToAngle(float degrees);

   void TurnToFacePoint(const csg::Point3& location);   

   csg::Cube3f GetWorldAABB() const;
   csg::Cube3f GetAABB() const;
   void SetAABB(const csg::Cube3f& box) { aabb_.Modify() = box; }

   csg::Point3f GetLocation() const;

   csg::Quaternion GetRotation() const;
   void TurnTo(const csg::Quaternion& orientation);

   csg::Point3f GetLocationInFront() const;
   csg::Transform GetTransform() const;
   bool GetMoving() const { return moving_; }
   MobPtr GetParent() const { return (*parent_).lock(); }

   Mob& SetMoving(bool m);
   Mob& SetTransform(csg::Transform const& t);

   csg::Point3 GetGridLocation() const;

   csg::Point3f GetWorldLocation() const;
   csg::Point3 GetWorldGridLocation() const;
   csg::Transform GetWorldTransform() const;

   bool InterpolateMovement() const;
   bool IsSelectable() const;
   void SetInterpolateMovement(bool value);

   dm::Boxed<csg::Transform> const& GetBoxedTransform() const { return transform_; }
   dm::Boxed<bool> const& GetBoxedMoving() const { return moving_; }

private:
   friend EntityContainer;
   void SetParent(MobPtr parent);

private:
   enum Flags {
      INTERPOLATE_MOVEMENT = (1 << 1),
      SELECTABLE           = (1 << 2)
   };
   void InitializeRecordFields() override;

public:
   dm::Boxed<MobRef>             parent_;
   dm::Boxed<csg::Transform>     transform_;
   dm::Boxed<csg::Cube3f>        aabb_;
   dm::Boxed<int>                flags_;
   dm::Boxed<bool>               moving_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_MOB_H
