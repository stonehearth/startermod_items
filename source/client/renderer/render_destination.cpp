#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_destination.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderDestination::RenderDestination(const RenderEntity& entity, om::DestinationPtr destination)
{
   H3DNode node = entity.GetNode();
   region_guard_   = CreateRegionDebugShape(node, regionDebugShape_,   destination->GetRegion(), csg::Color4(128, 128, 128, 128));
   //adjacent_guard_ = CreateRegionDebugShape(node, adjacentDebugShape_, destination->GetAdjacent(), csg::Color4(0, 0, 0, 128));
}

RenderDestination::~RenderDestination()
{
}
