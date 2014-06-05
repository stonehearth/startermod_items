#include "pch.h"
#include "render_mob.h"
#include "render_entity.h"
#include "om/components/mob.ridl.h"
#include "lib/perfmon/perfmon.h"
#include "renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define M_LOG(level)      LOG(renderer.mob, level)

RenderMob::RenderMob(const RenderEntity& entity, om::MobPtr mob) :
   entity_(entity),
   mob_(mob)
{
   ASSERT(mob);

   entity_id_ = entity.GetObjectId();

   show_debug_shape_guard_ += Renderer::GetInstance().OnShowDebugShapesChanged([this](bool enabled) {
      if (enabled) {
         RenderAxes();
      } else {
         RemoveAxes();
      }
   });

   interp_trace_ = mob->TraceInterpolateMovement("render", dm::RENDER_TRACES)
                                 ->OnChanged([this](bool interpolate) {
                                    UpdateInterpolate(interpolate);
                                 })
                                 ->PushObjectState();

   transform_trace_ = mob->TraceTransform("render", dm::RENDER_TRACES)
                                 ->OnChanged([this](csg::Transform const& transform) {
                                    UpdateTransform(transform);
                                 })
                                 ->PushObjectState();
   Move();
}

void RenderMob::RenderAxes()
{
   float d = 2;
   H3DNode s = h3dRadiantAddDebugShapes(entity_.GetOriginNode(), "mob debug axes");
   h3dRadiantAddDebugLine(s, csg::Point3f::zero, csg::Point3f(d, 0, 0), csg::Color4(255, 0, 0, 255));
   h3dRadiantAddDebugLine(s, csg::Point3f::zero, csg::Point3f(0, d, 0), csg::Color4(0, 255, 0, 255));
   h3dRadiantAddDebugLine(s, csg::Point3f::zero, csg::Point3f(0, 0, d), csg::Color4(0, 0, 255, 255));
   h3dRadiantCommitDebugShape(s);
   _axes.reset(s);
 }

void RenderMob::RemoveAxes()
{
   _axes.reset(0);
}

void RenderMob::Move()
{
   // xxx - we shouldn't need this Normalize, but without it the following assert will
   // occasionally fail in release builds.  something happened to this vector... perhaps
   // it got tweaked slightly when being sent over the netwwork, or maybe our value of
   // epislon is just too small.
   _current.orientation.Normalize();
   ASSERT(_current.orientation.is_unit());

   // Move the local origin of the render entity to the exact position of the mob.
   h3dSetNodeTransform(entity_.GetOriginNode(), _current.position.x, _current.position.y, _current.position.z, 0, 0, 0, 1, 1, 1);

   // Offset the node for the entity (which contains all the renderables) by the local
   // origin, the render offset, and the current rotation.
   csg::Matrix4 m;   
   om::MobPtr mob = mob_.lock();
   if (mob) {
      csg::Point3f const& renderOffset = mob->GetRenderOffset();
      csg::Point3f localOrigin = mob->GetLocalOrigin();
      csg::Matrix4 localOriginInvMat, renderOffsetMat;

      renderOffsetMat.set_translation(renderOffset);        // Move over to the render offset
      localOriginInvMat.set_translation(-localOrigin);      // Inverse of the local origin
      csg::Matrix4 rotation(_current.orientation);          // Rotate...

      // Look ma!  Matrix accumulation!
      m = renderOffsetMat * rotation * localOriginInvMat;
      M_LOG(9) << "render offset for " << *entity_.GetEntity() << " is " << renderOffset << " computed:" << m.get_translation();
   }
   M_LOG(9) << "transform for " << *entity_.GetEntity() << " is " << _current.position;

   bool result = h3dSetNodeTransMat(entity_.GetNode(), m.get_float_ptr());
   if (!result) {
      M_LOG(1) << "failed to set transform on node " << entity_.GetNode() << ".";
   }
}

void RenderMob::UpdateTransform(csg::Transform const& transform)
{
   auto mob = mob_.lock();
   if (mob) {
      om::EntityPtr current = parent_.lock();
      om::EntityPtr parent = mob->GetParent().lock();

      if (mob->GetInterpolateMovement()) {
         if (current != parent) {
            M_LOG(7) << "mob: turning off interpolation for " << entity_id_ << " for one frame, since parent changed";
            _initial = _final = _current = mob->GetTransform();
            parent_ = parent;
         } else {
            // Otherwise, update _initial and _final such that we smoothly interpolate between
            // the current mob's position and their location on the server.
            _initial = _final;
            _final = mob->GetTransform();
         }
         M_LOG(7) << "mob: initial for object " << entity_id_ << " to " << _initial << " in update transform";
         M_LOG(7) << "mob: final   for object " << entity_id_ << " to " << _final << " in update transform (stored value)";
      } else {
         _current = mob->GetTransform();
         M_LOG(7) << "mob: current for object " << entity_id_ << " to " << _current << " in update transform (stored value, interp off)";
         Move();
      }
   }
}

void RenderMob::UpdateInterpolate(bool interpolate)
{
   if (interpolate) {
      renderer_guard_ += Renderer::GetInstance().OnServerTick([this](int now) {
         _initial = _current;
         M_LOG(7) << "mob: initial for object " << entity_id_ << " to " << _current << " in server tick";
      });
      renderer_guard_ += Renderer::GetInstance().OnRenderFrameStart([this](FrameStartInfo const& info) {
         perfmon::TimelineCounterGuard tcg("move mob");
         _current = csg::Interpolate(_initial, _final, info.interpolate);
         M_LOG(7) << "mob: current for object " << entity_id_ << " to " << _current << " (a:" << info.interpolate << " n:" << info.now << " ft:" << info.frame_time << ")";
         Move();
      });
   } else {
      renderer_guard_.Clear();
   }
}
