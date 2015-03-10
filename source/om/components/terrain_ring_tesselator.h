#ifndef RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H
#define RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H

#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

class TerrainRingTesselator
{
public:
   TerrainRingTesselator();
   void LoadFromJson(json::Node config);
   csg::Region3f Tesselate(csg::Region3f const& terrain, csg::Rect2f const* clipper);
   csg::Region3 Tesselate(csg::Region3 const& terrain, csg::Rect2 const* clipper);

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

   void AddTerrainTypeToTesselation(csg::Region3 const& grass, csg::Rect2 const* clipper, RingInfo const& ringInfo, csg::Region3& tess) const;
   void TesselateLayer(csg::Region2 const& layer, int height, csg::Rect2 const* clipper, RingInfo const& ringInfo, csg::Region3& tess) const;
   csg::Region2 CreateRing(csg::EdgeMap2 const& edges, int width, csg::Region2 const* clipper) const;
   int GetTag(std::string name) const;

   std::unordered_map<std::string, int> tag_map_;
   std::unordered_map<int, RingInfo> ring_infos_;
};

typedef std::shared_ptr<TerrainRingTesselator> TerrainRingTesselatorPtr;

END_RADIANT_OM_NAMESPACE

std::ostream& operator<<(std::ostream& os, radiant::om::TerrainRingTesselator const& o);

#endif // RADIANT_OM_COMPONENTS_TERRAIN_TESSELATOR_H