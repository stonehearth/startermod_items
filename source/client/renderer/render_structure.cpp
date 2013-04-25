#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_structure.h"
#include "om/components/structure.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderStructure::RenderStructure(const RenderEntity& entity, om::StructurePtr structure) :
   entity_(entity)
{
   H3DNode parent = entity_.GetNode();
   std::string name = h3dGetNodeParamStr(parent, H3DNodeParams::NameStr) + std::string(" stockile designation");

   regionDebugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());
   reservedDebugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());

   traces_ += structure->TraceRegion("render structure",
                                     std::bind(&RenderStructure::UpdateRegion,
                                               this,
                                               std::placeholders::_1));
   traces_ += structure->TraceReserved("render structure",
                                       std::bind(&RenderStructure::UpdateReserved,
                                                 this,
                                                 std::placeholders::_1));
   UpdateRegion(structure->GetRegion());
   UpdateReserved(structure->GetReservedRegion());
}

RenderStructure::~RenderStructure()
{
}

void RenderStructure::UpdateRegion(const csg::Region3& region)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(regionDebugShape_);
      h3dRadiantAddDebugRegion(regionDebugShape_, region, math3d::color4(92, 92, 92, 192));
      h3dRadiantCommitDebugShape(regionDebugShape_);
   }
}

void RenderStructure::UpdateReserved(const csg::Region3& region)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(reservedDebugShape_);
      h3dRadiantAddDebugRegion(reservedDebugShape_, region, math3d::color4(255, 92, 92, 192));
      h3dRadiantCommitDebugShape(reservedDebugShape_);
   }
}


