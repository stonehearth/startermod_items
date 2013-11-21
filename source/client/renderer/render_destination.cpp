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
         RenderDestinationRegion();
      } else {
         RemoveDestinationRegion();
      }
   });
}

void RenderDestination::RenderDestinationRegion()
{
   H3DNode node = entity_->GetNode();
   region_trace_ = CreateRegionDebugShape(node, regionDebugShape_, destination_->GetRegion(), csg::Color4(128, 128, 128, 128));
}

void RenderDestination::RemoveDestinationRegion()
{
   region_trace_ = nullptr;
   regionDebugShape_.reset(0);
}

RenderDestination::~RenderDestination()
{
}
