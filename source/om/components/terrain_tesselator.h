#ifndef RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H
#define RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H

#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

class TerrainTesselator
{
public:
   TerrainTesselator();
   void TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess);

private:
   struct RingInfo {
      struct Ring {
         int width;
         int tag;
         Ring(int w, int t) : width(w), tag(t) {}
      };
      std::vector<Ring> rings;
      int innerTag;
   };

   RingInfo grass_ring_info_;
   RingInfo dirt_ring_info_;

   void AddTerrainTypeToTesselation(csg::Region3 const& grass, csg::Region3 const& clipper, RingInfo const& ringInfo, csg::Region3& tess);
   void TesselateLayer(csg::Region2 const& layer, int height, csg::Region3 const& clipper, RingInfo const& ringInfo, csg::Region3& tess);
};
END_RADIANT_OM_NAMESPACE

#endif // RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H
