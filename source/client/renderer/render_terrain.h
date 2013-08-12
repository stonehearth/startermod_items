#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_H

#include <map>
#include "namespace.h"
#include "render_component.h"
#include "types.h"
#include "om/om.h"
#include "dm/dm.h"
#include "csg/util.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderTerrain : public RenderComponent {
public:
   RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain);

private:
   enum TerrainDetailTypes {
      RenderDetailBase = 1000,

      FoothillsDetailBase = RenderDetailBase,
      FoothillsDetailMax  = FoothillsDetailBase + 4,

      GrassDetailBase,
      GrassDetailMax  = GrassDetailBase + 4,

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
   void UpdateRenderRegion();
   void TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess);
   void OnSelected(om::Selection& sel, const math3d::ray3& ray,
                   const math3d::point3& intersection, const math3d::point3& normal);

   void AddGrassToTesselation(csg::Region3 const& grass, csg::Region3 const& terrain, csg::Region3& tess, LayerDetailRingInfo const &rings);
   void AddGrassLayerToTesselation(csg::Region2 const& grass, int height, csg::Region3 const& clipper, csg::Region3& tess, LayerDetailRingInfo const &rings);

   static LayerDetailRingInfo foothillGrassRingInfo_;
   static LayerDetailRingInfo plainsGrassRingInfo_;
   static LayerDetailRingInfo dirtRoadRingInfo_;

private:
   const RenderEntity&  entity_;
   dm::Guard            tracer_;
   om::TerrainRef       terrain_;
   H3DNode              node_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
