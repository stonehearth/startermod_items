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

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderTerrain : public RenderComponent {
public:
   RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain);
   ~RenderTerrain();

private:
   enum TerrainDetailTypes {
      RenderDetailBase = 1000,

      LightGrassDetailBase = RenderDetailBase,
      LightGrassDetailMax  = LightGrassDetailBase + 2,

      DarkGrassDetailBase,
      DarkGrassDetailMax = DarkGrassDetailBase + 2,

      DirtRoadBase,
      DirtRoadMax = DirtRoadBase + 1,

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
   struct RenderZone {
      csg::Point3                location;
      om::Region3BoxedRef        region;
      H3DNodeUnique              node;
      core::Guard                  guard;

      RenderZone() { }
      void Reset() {
         node.reset(0);
      }
   };
   DECLARE_SHARED_POINTER_TYPES(RenderZone)

private:
   void Update();
   void UpdateRenderRegion(RenderZonePtr render_zone);
   void TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess);
   void OnSelected(om::Selection& sel, const csg::Ray3& ray,
                   const csg::Point3f& intersection, const csg::Point3f& normal);

   void AddTerrainTypeToTesselation(csg::Region3 const& grass, csg::Region3 const& terrain, csg::Region3& tess, LayerDetailRingInfo const &rings);
   void TesselateLayer(csg::Region2 const& layer, int height, csg::Region3 const& clipper, csg::Region3& tess, LayerDetailRingInfo const &rings);

   static LayerDetailRingInfo light_grass_ring_info_;
   static LayerDetailRingInfo dark_grass_ring_info_;
   static LayerDetailRingInfo dirt_road_ring_info_;

private:
   const RenderEntity&  entity_;
   core::Guard            tracer_;
   om::TerrainRef       terrain_;
   H3DNodeUnique        terrain_root_node_;
   std::map<csg::Point3, std::shared_ptr<RenderZone>>   zones_;
   std::vector<RenderZoneRef> dirty_zones_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
