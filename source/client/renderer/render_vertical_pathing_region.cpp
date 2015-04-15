#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_vertical_pathing_region.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderVerticalPathingRegion::RenderVerticalPathingRegion(const RenderEntity& entity, om::VerticalPathingRegionPtr vertical_pathing_region)
   : entity_(entity)
{
   vertical_pathing_region_ = vertical_pathing_region;

   renderer_guard_ += Renderer::GetInstance().OnShowDebugShapesChanged([this](dm::ObjectId id) {
      bool enabled;
      if (id <= 0) {
         enabled = (id < 0);
      } else {
         om::EntityPtr entity = entity_.GetEntity();
         enabled = (entity && entity->GetObjectId() == id);
      }
      if (enabled) {
         region_trace_ = vertical_pathing_region_->TraceRegion("debug rendering", dm::RENDER_TRACES);
         CreateRegionDebugShape(entity_.GetEntity(), regionDebugShape_, region_trace_, csg::Color4(0, 0, 192, 12));
      } else {
         region_trace_ = nullptr;
         regionDebugShape_.reset(0);
      }
   });
}

RenderVerticalPathingRegion::~RenderVerticalPathingRegion()
{
}
