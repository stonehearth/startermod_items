#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_destination.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderDestination::RenderDestination(const RenderEntity& entity, om::DestinationPtr destination) :
   entity_(entity)
{
   node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "destination region"));
 
   //region_guard_ = create_debug_tracker("region",   regionDebugShape_,   destination->GetRegion(),   csg::Color4(0, 128, 255, 128));
   //reserved_guard_ = create_debug_tracker("reserved", reservedDebugShape_, destination->GetReserved(), csg::Color4(255, 128, 0, 128));
   //adjacent_guard_ = create_debug_tracker("adjacent", adjacentDebugShape_, destination->GetAdjacent(), csg::Color4(255, 255, 255, 128));
   CreateDebugTracker(region_guard_,   "region",   regionDebugShape_,   destination->GetRegion(),   csg::Color4(128, 128, 128, 128));
   CreateDebugTracker(adjacent_guard_, "adjacent", adjacentDebugShape_, destination->GetAdjacent(), csg::Color4(255, 255, 255, 128));
}

RenderDestination::~RenderDestination()
{
}

void RenderDestination::CreateDebugTracker(om::BoxedRegionGuardPtr& guard,
                                           std::string const& regionName,
                                           H3DNodeUnique& shape,
                                           dm::Boxed<om::BoxedRegion3Ptr> const& region,
                                           csg::Color4 const& color)
{
   H3DNode node = node_.get();
   std::string nodeName = h3dGetNodeParamStr(node, H3DNodeParams::NameStr);

   H3DNode s = h3dRadiantAddDebugShapes(node, (nodeName + regionName, std::string(" destination debug shape")).c_str());
   shape = H3DNodeUnique(s);

   guard = om::TraceBoxedRegion3PtrField(region, "rendering destination debug region", [this, s, color, &region](csg::Region3 const& r) {
      LOG(WARNING) << "client updating shape " << region << " " << r << " " << region.GetStoreId() << ":" << region.GetObjectId() << " " << entity_.GetEntity()->GetObjectId();
      LOG(INFO) << dm::DbgInfo::GetInfoString(region);
      UpdateShape(r, s, color);
   });   
   if (*region) {
      dm::ObjectId id = (*region)->GetObjectId();
      UpdateShape(**region, s, color);
   }
}


void RenderDestination::UpdateShape(csg::Region3 const& region, H3DNode shape, csg::Color4 const& color)
{
   dm::ObjectId id = entity_.GetEntity()->GetObjectId();
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(shape);
      h3dRadiantAddDebugRegion(shape, region, color);
      h3dRadiantCommitDebugShape(shape);
   }
}
