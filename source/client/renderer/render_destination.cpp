#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_destination.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderDestination::RenderDestination(const RenderEntity& entity, om::DestinationPtr destination) :
   entity_(entity)
{
   node_ = h3dAddGroupNode(entity_.GetNode(), "destination region");
   std::string nodeName = h3dGetNodeParamStr(node_, H3DNodeParams::NameStr);

   auto create_debug_tracker = [=](std::string regionName, H3DNode& shape, om::BoxedRegion3Ptr region, csg::Color4 const& color) {
      shape = h3dRadiantAddDebugShapes(node_, (nodeName + regionName, std::string(" destination debug shape")).c_str());
      auto update = std::bind(&RenderDestination::UpdateShape, this, om::BoxedRegion3Ref(region), shape, color);
      guards_ += region->TraceObjectChanges("rendering destination debug region", update);
      update(**region);
   };

   create_debug_tracker("region", regionDebugShape_, destination->GetRegion(), csg::Color4(0, 128, 255, 128));
   create_debug_tracker("reserved", reservedDebugShape_, destination->GetReserved(), csg::Color4(255, 128, 0, 128));
   create_debug_tracker("adjacent", adjacentDebugShape_, destination->GetAdjacent(), csg::Color4(255, 255, 255, 128));
}

RenderDestination::~RenderDestination()
{
}

void RenderDestination::UpdateShape(om::BoxedRegion3Ref r, H3DNode shape, csg::Color4 const& color)
{
   if (entity_.ShowDebugRegions()) {
      auto region = r.lock();
      if (region) {
         h3dRadiantClearDebugShape(shape);
         h3dRadiantAddDebugRegion(shape, region->Get(), color);
         h3dRadiantCommitDebugShape(shape);
      }
   }
}
