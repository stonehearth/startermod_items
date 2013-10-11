#include "pch.h"
#include "render_entity.h"
#include "renderer.h"
#include "render_vertical_pathing_region.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderVerticalPathingRegion::RenderVerticalPathingRegion(const RenderEntity& entity, om::VerticalPathingRegionPtr vertical_pathing_region)
{
   region_guard_ = CreateRegionDebugShape(entity.GetNode(), regionDebugShape_, vertical_pathing_region->GetRegion(), csg::Color4(0, 0, 192, 128));
}

RenderVerticalPathingRegion::~RenderVerticalPathingRegion()
{
}
