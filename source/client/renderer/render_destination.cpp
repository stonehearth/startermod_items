#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_destination.h"
#include "om/components/stockpile_designation.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderDestination::RenderDestination(const RenderEntity& entity, om::DestinationPtr stockpile) :
   entity_(entity)
{
   node_ = h3dAddGroupNode(entity_.GetNode(), "destination region");
   h3dSetNodeTransform(node_, 0.5f, 0.0f, 0.5f, 0, 0, 0, 1, 1, 1);
   std::string nodeName = h3dGetNodeParamStr(node_, H3DNodeParams::NameStr);

   auto create_debug_tracker = [=](std::string regionName, H3DNode& shape, om::RegionPtr region, math3d::color4 const& color) {
      shape = h3dRadiantAddDebugShapes(node_, (nodeName + regionName, std::string(" destination debug shape")).c_str());
      auto update = std::bind(&RenderDestination::UpdateShape, this, std::placeholders::_1, shape, color);
      guards_ += region->Trace("rendering destination debug region", update);
      update(**region);
   };

   create_debug_tracker("region", regionDebugShape_, stockpile->GetRegion(), math3d::color4(0, 128, 255, 128));
   create_debug_tracker("reserved", reservedDebugShape_, stockpile->GetReserved(), math3d::color4(255, 128, 0, 128));
   create_debug_tracker("adjacent", adjacentDebugShape_, stockpile->GetAdjacent(), math3d::color4(255, 255, 255, 128));
}

RenderDestination::~RenderDestination()
{
}

void RenderDestination::UpdateShape(csg::Region3 const& region, H3DNode shape, math3d::color4 const& color)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(shape);
      h3dRadiantAddDebugRegion(shape, region, color);
      h3dRadiantCommitDebugShape(shape);
   }
}
