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

private:
   class RenderTile {
   public:
      csg::Point3                location;
      om::Region3fBoxedRef       region;
      dm::TracePtr               trace;

      RenderTile() { }

      void SetNode(RenderNodePtr n) {
         _node = n;
      }

   private:
      RenderNodePtr _node;
   };
   DECLARE_SHARED_POINTER_TYPES(RenderTile)

private:
   void InitalizeColorMap();
   void Update();
   void UpdateRenderRegion(RenderTilePtr render_tile);
   void AddDirtyTile(RenderTileRef tile);

private:
   const RenderEntity&  entity_;
   dm::TracePtr         tiles_trace_;
   core::Guard          selected_guard_;
   om::TerrainRef       terrain_;
   H3DNodeUnique        terrain_root_node_;
   std::unordered_map<csg::Point3, std::shared_ptr<RenderTile>, csg::Point3::Hash> tiles_;
   std::vector<RenderTileRef> dirty_tiles_;
   core::Guard          renderer_frame_trace_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
