#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_build_order.h"
#include "om/components/build_order.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderBuildOrder::RenderBuildOrder(const RenderEntity& entity, om::BuildOrderPtr buildOrder) :
   entity_(entity)
{
   H3DNode parent = entity_.GetNode();
   std::string name = h3dGetNodeParamStr(parent, H3DNodeParams::NameStr) + std::string(" build order");

   regionDebugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());
   reservedDebugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());

   traces_ += buildOrder->TraceRegion("render build_order region", [=](const csg::Region3& region) {
      UpdateRegion(region);
   });
   traces_ += buildOrder->TraceReserved("render build_order reserved", [=](const csg::Region3& region) {
      UpdateReserved(region);
   });

   UpdateRegion(buildOrder->GetRegion());
   UpdateReserved(buildOrder->GetReservedRegion());
}

RenderBuildOrder::~RenderBuildOrder()
{
}

void RenderBuildOrder::UpdateRegion(const csg::Region3& region)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(regionDebugShape_);
      h3dRadiantAddDebugRegion(regionDebugShape_, region, math3d::color4(92, 92, 92, 192));
      h3dRadiantCommitDebugShape(regionDebugShape_);
   }
}


void RenderBuildOrder::UpdateReserved(const csg::Region3& region)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(reservedDebugShape_);
      h3dRadiantAddDebugRegion(reservedDebugShape_, region, math3d::color4(255, 92, 92, 192));
      h3dRadiantCommitDebugShape(reservedDebugShape_);
   }
}


H3DNode RenderBuildOrder::GetParentNode() const
{
   return entity_.GetNode();
}
