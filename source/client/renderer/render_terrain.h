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
   enum TerrainDetailTypes {
      RenderDetailBase = 1000,

      GrassDetailBase = RenderDetailBase,
      GrassDetailMax  = GrassDetailBase + 2,

      DirtBase,
      DirtMax = DirtBase + 1,

      RenderDetailMax
   };

   struct LayerDetailRingInfo {
      struct Ring {
         int                  width;
         TerrainDetailTypes   type;
         Ring(int w, TerrainDetailTypes t) : width(w), type(t) { }
         Ring(int w, int t) : width(w), type((TerrainDetailTypes)t) { }
      };
      std::vector<Ring>       rings;
      TerrainDetailTypes      inner;
   };

private:
   class RenderTile {
   public:
      csg::Point3                location;
      om::Region3BoxedRef        region;
      dm::TracePtr               trace;

      RenderTile() { }

      void SetNode(RenderNode n) {
         _node = n;
      }

   private:
      RenderNode _node;
   };
   DECLARE_SHARED_POINTER_TYPES(RenderTile)

private:
   void Update();
   void UpdateRenderRegion(RenderTilePtr render_tile);
   void TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess);

   void AddTerrainTypeToTesselation(csg::Region3 const& grass, csg::Region3 const& terrain, csg::Region3& tess, LayerDetailRingInfo const &rings);
   void TesselateLayer(csg::Region2 const& layer, int height, csg::Region3 const& clipper, csg::Region3& tess, LayerDetailRingInfo const &rings);
   void AddDirtyTile(RenderTileRef tile);

   static LayerDetailRingInfo grass_ring_info_;
   static LayerDetailRingInfo dirt_ring_info_;

private:
   const RenderEntity&  entity_;
   dm::TracePtr         tiles_trace_;
   core::Guard          selected_guard_;
   om::TerrainRef       terrain_;
   H3DNodeUnique        terrain_root_node_;
   std::map<csg::Point3, std::shared_ptr<RenderTile>>   tiles_;
   std::vector<RenderTileRef> dirty_tiles_;
   core::Guard          renderer_frame_trace_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
