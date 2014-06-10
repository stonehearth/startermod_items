#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_region_collision_shape.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderRegionCollisionShape::RenderRegionCollisionShape(const RenderEntity& entity, om::RegionCollisionShapePtr collision_shape) :
   entity_(entity)
{
   collision_shape_ = collision_shape;

   renderer_guard_ = Renderer::GetInstance().OnShowDebugShapesChanged([this](bool enabled) {
      if (enabled) {
         auto collision_shape = collision_shape_.lock();
         if (collision_shape) {
            trace_ = collision_shape->TraceRegion("debug rendering", dm::RENDER_TRACES);
            CreateRegionDebugShape(entity_.GetEntity(), shape_, trace_, csg::Color4(32, 32, 32, 128));
         }
      } else {
         trace_ = nullptr;
         shape_.reset(0);
      }
   });
}
