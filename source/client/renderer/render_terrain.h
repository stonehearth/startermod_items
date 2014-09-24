#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_H

#include <map>
#include "namespace.h"
#include "forward_defines.h"
#include "render_component.h"
#include "h3d_resource_types.h"
#include "om/om.h"
#include "dm/dm.h"
#include "csg/util.h"
#include "render_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderTerrain : public RenderComponent {
public:
   RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain);
   ~RenderTerrain();

   csg::TagToColorMap const& GetColorMap() const;
   H3DNode GetGroupNode() const;
   void MarkDirty(RenderTerrainTileRef tile);

private:
   void InitalizeColorMap();
   void Update();

private:
   const RenderEntity&  entity_;
   dm::TracePtr         tiles_trace_;
   core::Guard          selected_guard_;
   om::TerrainRef       terrain_;
   H3DNodeUnique        terrain_root_node_;
   std::unordered_map<csg::Point3, RenderTerrainTilePtr, csg::Point3::Hash> tiles_;
   std::vector<RenderTerrainTileRef> dirty_tiles_;
   core::Guard          renderer_frame_trace_;
   csg::TagToColorMap   _colorMap;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
