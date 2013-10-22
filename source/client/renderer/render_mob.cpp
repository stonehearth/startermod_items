#include "pch.h"
#include "render_mob.h"
#include "render_entity.h"
#include "om/components/mob.h"
#include "renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderMob::RenderMob(const RenderEntity& entity, om::MobPtr mob) :
   entity_(entity),
   mob_(mob)
{
   ASSERT(mob);

   tracer_ += Renderer::GetInstance().OnShowDebugShapesChanged([this](bool enabled) {
      if (enabled) RenderAxes();
      else         RemoveAxes();
   });
   
   tracer_ += mob->TraceRecordField("transform", "move render entity", std::bind(&RenderMob::Update, this));
   if (mob->InterpolateMovement()) {
      tracer_ += Renderer::GetInstance().TraceFrameStart(std::bind(&RenderMob::Interpolate, this));
      tracer_ += Renderer::GetInstance().TraceInterpolationStart(std::bind(&RenderMob::StartInterpolate, this));
   }
   _current = _initial = _final = mob->GetTransform();

   Move();
}

void RenderMob::RenderAxes()
{
   float d = 1.5;
   H3DNode s = h3dRadiantAddDebugShapes(entity_.GetNode(), "mob debug axes");
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

   csg::Matrix4 m(_current.orientation);
   
   m[12] = _current.position.x;
   m[13] = _current.position.y;
   m[14] = _current.position.z;

   bool result = h3dSetNodeTransMat(entity_.GetNode(), m.get_float_ptr());
   if (!result) {
      LOG(WARNING) << "failed to set transform on node.";
   }
}

void RenderMob::Update()
{
   auto mob = mob_.lock();
   if (mob) {
      if (mob->InterpolateMovement()) {
         _final = mob->GetTransform();
      } else {
         _current = mob->GetTransform();
         Move();
      }
   }
}

void RenderMob::Interpolate()
{
   float alpha = Renderer::GetInstance().GetCurrentFrameInterp();
   _current = csg::Interpolate(_initial, _final, alpha);
   Move();
}

void RenderMob::StartInterpolate()
{
   _initial = _current;
}
