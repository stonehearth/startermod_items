#include "pch.h"
#include "render_mob.h"
#include "render_entity.h"
#include "om/components/mob.ridl.h"
#include "lib/perfmon/perfmon.h"
#include "physics/physics_util.h"
#include "renderer.h"
#include "render_node.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define M_LOG(level)      LOG(renderer.mob, level)

RenderMob::RenderMob(RenderEntity& entity, om::MobPtr mob) :
   entity_(entity),
   mob_(mob)
{
   ASSERT(mob);
   _final = mob->GetTransform();
   _current = _final;
   entity_id_ = entity.GetObjectId();

   show_debug_shape_guard_ += Renderer::GetInstance().OnShowDebugShapesChanged([this](dm::ObjectId id) {
      bool enabled;
      if (id <= 0) {
         enabled = (id < 0);
      } else {
         om::EntityPtr entity = entity_.GetEntity();
         enabled = (entity && entity->GetObjectId() == id);
      }

      if (enabled) {
         RenderAxes();
      } else {
         RemoveAxes();
      }
   });

   if (!entity_.GetParentOverride()) {
      parent_trace_ = mob->TraceParent("render", dm::RENDER_TRACES)
                                    ->OnChanged([this](om::EntityRef parent) {
                                       UpdateParent();
                                    })
                                    ->PushObjectState();

      bone_trace_ = mob->TraceBone("render", dm::RENDER_TRACES)
                                    ->OnChanged([this](std::string const&) {
                                       UpdateParent();
                                    })
                                    ->PushObjectState();
   }

   interp_trace_ = mob->TraceInterpolateMovement("render", dm::RENDER_TRACES)
                                 ->OnChanged([this](bool) {
                                    UpdateInterpolate();
                                 });

   _freeMotionTrace = mob->TraceInFreeMotion("render", dm::RENDER_TRACES)
                                 ->OnChanged([this](bool) {
                                    UpdateInterpolate();
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
   ASSERT(_current.position == _current.position);

   // Move the local origin of the render entity to the exact position of the mob.
   h3dSetNodeTransform(entity_.GetOriginNode(), (float)_current.position.x, (float)_current.position.y, (float)_current.position.z, 0, 0, 0, 1, 1, 1);

   // Offset the node for the entity (which contains all the renderables) by the local
   // origin, the render offset, and the current rotation.
   csg::Matrix4 m;   
   om::MobPtr mob = mob_.lock();
   if (mob) {
      csg::Point3f renderOffset(0, 0, 0);
      csg::Point3f modelOrigin = mob->GetModelOrigin();
      int alignment = mob->GetAlignToGridFlags();

      for (int i = 0; i < 3; i++) {
         if (alignment & (1 << i)) {
            renderOffset[i] = phys::GetTerrainAlignmentOffset(modelOrigin[i]);
         }
      }
      renderOffset = _current.orientation.rotate(renderOffset);
      for (int i = 0; i < 3; i++) {
         if (renderOffset[i] > 0) {
            renderOffset[i] *= -1;
         }
      }

      csg::Matrix4 modelOriginInvMat, renderOffsetMat;

      renderOffsetMat.set_translation(renderOffset);        // Move over to the render offset
      modelOriginInvMat.set_translation(-modelOrigin);      // Inverse of the local origin
      csg::Matrix4 rotation(_current.orientation);          // Rotate...

      // Look ma!  Matrix accumulation!
      m = renderOffsetMat * rotation * modelOriginInvMat;
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
      if (InterpolateMovement()) {
         om::EntityPtr current = lastParent_.lock();
         om::EntityPtr parent = mob->GetParent().lock();

         if (current != parent) {
            M_LOG(7) << "mob: turning off interpolation for " << entity_id_ << " for one frame, since parent changed";
            _initial = _final = _current = mob->GetTransform();
            lastParent_ = parent;
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

void RenderMob::UpdateInterpolate()
{
   if (InterpolateMovement()) {
      if (renderer_guard_.Empty()) {
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
      }
   } else {
      renderer_guard_.Clear();
   }
}

void RenderMob::UpdateParent()
{
   if (entity_.GetParentOverride()) {
      bone_trace_.reset();
      parent_trace_.reset();
      return;
   }

   om::MobPtr mob = mob_.lock();
   if (mob) {
      H3DNode unparented = RenderNode::GetUnparentedNode();
      om::EntityPtr parent = mob->GetParent().lock();

      H3DNode parentNode = unparented;
      if (parent) {
         // Get the render entity for our parent.  At startup, we might actually get loaded
         // before them, so create it under the unparented node if necessary.
         RenderEntityPtr re = Renderer::GetInstance().GetRenderEntity(parent);
         if (!re) {
            re = Renderer::GetInstance().CreateRenderEntity(unparented, parent);
         }

         // If we're attached to a bone on the parent, use the appropriate horde node from
         // its skeleton.  Otherwise, attach to the origin.
         std::string const& bone = mob->GetBone();
         if (bone.empty()) {
            parentNode = re->GetOriginNode();
            M_LOG(5) << "attaching " << *entity_.GetEntity() << " to " << *re->GetEntity();
         } else {
            parentNode = re->GetSkeleton().GetSceneNode(bone);
            M_LOG(5) << "attaching " << *entity_.GetEntity() << " to \"" << bone << "\" bone on " << *re->GetEntity();
         }
      } else {
         M_LOG(5) << "attaching " << *entity_.GetEntity() << " to unparented render node";
      }
      entity_.SetParent(parentNode);
   }
}

bool RenderMob::InterpolateMovement() const
{
   om::MobPtr mob = mob_.lock();

   if (mob) {
      return mob->GetInterpolateMovement() || mob->GetInFreeMotion();
   }
   return false;
}
