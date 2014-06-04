#include "pch.h"
#include "region.h"
#include "csg/util.h"
#include "csg/matrix4.h"
#include "csg/transform.h"
#include "csg/rotate_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define DRG_LOG(level)   LOG(om.region, level)

// xxx: why isn't all this bundled up in a "trace recursive" package?  Sounds like a good idea!!
dm::GenerationId om::DeepObj_GetLastModified(Region3BoxedPtrBoxed const& boxedRegionPtrField)
{
   dm::GenerationId modified = boxedRegionPtrField.GetLastModified();
   Region3BoxedPtr value = *boxedRegionPtrField;
   if (value) {
      modified = std::max<dm::GenerationId>(modified, value->GetLastModified());
   }
   return modified;
}


DeepRegionGuardPtr om::DeepTraceRegion(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                       const char* reason,
                                       int category)
{   
   DeepRegionGuardPtr result = std::make_shared<DeepRegionGuard>(boxedRegionPtrField.GetStoreId(),
                                                                 boxedRegionPtrField.GetObjectId());
   DeepRegionGuardRef r = result;

   auto boxed_trace = boxedRegionPtrField.TraceChanges(reason, category);
   result->boxed_trace = boxed_trace;

   DRG_LOG(5) << "tracing boxed region " << boxedRegionPtrField.GetObjectId() << " (" << reason << ")";

   boxed_trace->OnChanged([r, reason, category](Region3BoxedPtr value) {
         DRG_LOG(7) << "region pointer in box is now " << value;
         auto guard = r.lock();
         if (guard) {
            if (value) {
               DRG_LOG(7) << "installing new trace on " << value;
               auto region_trace = value->TraceChanges(reason, category);
               guard->region_trace = region_trace;

               region_trace
                  ->OnChanged([r](csg::Region3 const& region) {
                     DRG_LOG(7) << "value of region pointer in box changed.  signaling change cb with " << region;
                     auto guard = r.lock();
                     if (guard) {
                        guard->SignalChanged(region);
                     }
                  })
                  ->PushObjectState();
            } else {
               DRG_LOG(7) << "signalling change cb with empty region ";
               guard->SignalChanged(csg::Region3());
               guard->region_trace = nullptr;
            }
         }
      })
      ->PushObjectState();

   return result;
}

#pragma optimize( "", off )

/*
 * -- om::LocalToWorld
 *
 * Transform `shape` from the coordinate system of `enity` to the world coordinate
 * system, including transformation and rotation.
 *
 */
template <typename Shape>
Shape om::LocalToWorld(Shape const& shape, om::EntityPtr entity)
{
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return shape;
   }

   csg::Transform t = mob->GetWorldTransform();
   csg::Point3f localOrigin = mob->GetLocalOrigin();
   csg::Point3 position = csg::ToClosestInt(t.position);
   csg::Quaternion const& orientation = t.orientation;

   csg::Point3f axis;
   float radAngle;
   orientation.get_axis_angle(axis, radAngle);

   // Assumes rotation about the Y axis.
   float degrees = radAngle * 180 / csg::k_pi;
   int angle = (csg::ToClosestInt(degrees / 90) * 90) % 360;

   if (localOrigin != csg::Point3f::zero) {
      // Adjust the shape position based on our rotated local origin.  The local origin
      // is almost always specified as a fractional unit (e.g.  (1.5, 0, 9.5)) to 
      // position the render shape for the entity aligned to the grid.  Before we do
      // the offset, take that slop off so the shapes is integer aligned.
      csg::Point3f localOriginRotated = orientation.rotate(localOrigin);
      position -= csg::ToClosestInt(localOriginRotated - csg::Point3f(0.5f, 0, 0.5f));
   }
   if (angle == 0) {
      // If there's no rotation at all, we can just translate the shape to
      // the accumulated position.  Not calling csg::Rotated() here saves
      // a copy (which is important for regions!).
      return shape.Translated(position);
   }

   // Rotate the shape to the correct orientation and move it to the
   // world position.
   return csg::Rotated(shape, angle).Translated(position);
}

template <typename Shape>
Shape om::WorldToLocal(Shape const& shape, om::EntityPtr entity)
{
   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return shape;
   }

   csg::Transform t = mob->GetWorldTransform();
   csg::Matrix4 xform = t.GetMatrix();

   auto fShape = csg::ToFloat(shape);
   auto result = xform.affine_inverse().transform(fShape);
   return csg::ToInt(result);
}

template csg::Cube3 om::LocalToWorld(csg::Cube3 const&, om::EntityPtr);
template csg::Point3 om::LocalToWorld(csg::Point3 const&, om::EntityPtr);
template csg::Region3 om::LocalToWorld(csg::Region3 const&, om::EntityPtr);

template csg::Point3 om::WorldToLocal(csg::Point3 const&, om::EntityPtr);

#pragma optimize( "", on )


#if 0
Region3BoxedPromise::Region3BoxedPromise(Region3BoxedPtrBoxed const& boxedRegionPtrField, const char* reason)
{
   region_guard_ = DeepTraceRegion(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, r);
      }
   });
}

Region3BoxedPromise* Region3BoxedPromise::PushChangedCb(luabind::object cb) {
   changedCbs_.push_back(cb);
   return this;
}
#endif
