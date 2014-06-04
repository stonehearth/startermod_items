#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_destination.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderDestination::RenderDestination(const RenderEntity& entity, om::DestinationPtr destination)
{
   entity_ = &entity;
   destination_ = destination;

   renderer_guard_ += Renderer::GetInstance().OnShowDebugShapesChanged([this](bool enabled) {
      if (enabled) {
         RenderDestinationRegion(REGION, destination_->TraceRegion("debug rendering", dm::RENDER_TRACES), csg::Color4(128, 128, 128, 128));
         RenderDestinationRegion(RESERVED, destination_->TraceReserved("debug rendering", dm::RENDER_TRACES), csg::Color4(0, 255, 0, 128));
         RenderDestinationRegion(ADJACENT, destination_->TraceAdjacent("debug rendering", dm::RENDER_TRACES), csg::Color4(255, 0, 0, 128));
      } else {
         RemoveDestinationRegion(REGION);
         RemoveDestinationRegion(RESERVED);
         RemoveDestinationRegion(ADJACENT);
      }
   });
}

void RenderDestination::RenderDestinationRegion(int i, om::DeepRegionGuardPtr trace, csg::Color4 const& color)
{
   H3DNode node = entity_->GetNode();
   region_trace_[i] = trace;
   CreateRegionDebugShape(node, regionDebugShape_[i], region_trace_[i], color);
}

void RenderDestination::RemoveDestinationRegion(int i)
{
   region_trace_[i] = nullptr;
   regionDebugShape_[i].reset(0);
}

RenderDestination::~RenderDestination()
{
}
