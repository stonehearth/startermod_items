#include "radiant.h"
#include "physics_util.h"
#include "csg/util.h"
#include "csg/matrix4.h"
#include "csg/transform.h"
#include "csg/rotate_shape.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * -- phys::LocalToWorld
 *
 * Transform `shape` from the coordinate system of `entity` to the world coordinate
 * system, including transformation and rotation.
 *
 */
template <typename Shape>
Shape phys::LocalToWorld(Shape const& shape, om::EntityPtr entity)
{
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return shape;
   }

   csg::Transform t = mob->GetWorldTransform();
   csg::Point3f regionOrigin = mob->GetRegionOrigin();
   csg::Point3 position = csg::ToClosestInt(t.position);
   csg::Quaternion const& orientation = t.orientation;

   csg::Point3f axis;
   float radAngle;
   csg::GetAxisAngleNormalized(orientation, axis, radAngle);

   // Assumes rotation about the Y axis.
   float degrees = radAngle * 180 / csg::k_pi;
   int angle = (csg::ToClosestInt(degrees / 90) * 90) % 360;

   if (regionOrigin != csg::Point3f::zero) {
      // Adjust the shape position based on our rotated local origin.  The local origin
      // is almost always specified as a fractional unit (e.g.  (1.5, 0, 9.5)) to 
      // position the render shape for the entity aligned to the grid.  Before we do
      // the offset, take that slop off so the shapes is integer aligned.
      csg::Point3f regionOriginRotated = orientation.rotate(regionOrigin);
      position -= csg::ToClosestInt(regionOriginRotated - csg::Point3f(0.5f, 0, 0.5f));
   }
   if (angle == 0) {
      // If there's no rotation at all, we can just translate the shape to
      // the accumulated position.  Not calling csg::Rotated() here saves
      // a copy (which is important for regions!).
      return shape.Translated(csg::ConvertTo<Shape::ScalarType, 3>(position));
   }

   // Rotate the shape to the correct orientation and move it to the
   // world position.
   return csg::Rotated(shape, angle).Translated(csg::ConvertTo<Shape::ScalarType, 3>(position));
}


/*
 * -- phys::WorldToLocal
 *
 * Transform `shape` from world coordinate system to the coordinate system
 * of `entity` to the system, including transformation and rotation.
 *
 */
template <typename Shape>
Shape phys::WorldToLocal(Shape const& shape, om::EntityPtr entity)
{
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return shape;
   }

   csg::Transform t = mob->GetWorldTransform();
   csg::Matrix4 xform = t.GetMatrix();

   auto fShape = csg::ToFloat(shape);
   auto result = xform.affine_inverse().transform(fShape);
   return csg::ConvertTo<Shape::ScalarType, 3>(result);
}

bool phys::IsTerrainAligned(float modelOrigin)
{
   float unused;
   bool aligned = std::abs(std::modf(modelOrigin, &unused)) == 0.5f;
   return aligned;
}

// returns the translation needed to align the model boundaries to the terrain grid
float phys::GetTerrainAlignmentOffset(float modelOrigin)
{
   if (phys::IsTerrainAligned(modelOrigin)) {
      return 0.0f;
   } else {
      return -0.5f;
   }
}

template csg::Cube3 phys::LocalToWorld(csg::Cube3 const&, om::EntityPtr);
template csg::Point3 phys::LocalToWorld(csg::Point3 const&, om::EntityPtr);
template csg::Region3 phys::LocalToWorld(csg::Region3 const&, om::EntityPtr);
template csg::Cube3f phys::LocalToWorld(csg::Cube3f const&, om::EntityPtr);
template csg::Point3f phys::LocalToWorld(csg::Point3f const&, om::EntityPtr);
template csg::Region3f phys::LocalToWorld(csg::Region3f const&, om::EntityPtr);

template csg::Point3 phys::WorldToLocal(csg::Point3 const&, om::EntityPtr);
template csg::Point3f phys::WorldToLocal(csg::Point3f const&, om::EntityPtr);
