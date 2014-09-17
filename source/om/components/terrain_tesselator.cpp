#include "radiant.h"
#include "csg/util.h"
#include "terrain.ridl.h"
#include "terrain_tesselator.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

TerrainTesselator::TerrainTesselator()
{
   // TODO: make this configurable
   grass_ring_info_.rings.emplace_back(RingInfo::Ring(4, om::Terrain::GrassEdge1));
   grass_ring_info_.rings.emplace_back(RingInfo::Ring(6, om::Terrain::GrassEdge2));
   grass_ring_info_.innerTag = om::Terrain::Grass;

   dirt_ring_info_.rings.emplace_back(RingInfo::Ring(8, om::Terrain::DirtEdge1));
   dirt_ring_info_.innerTag = om::Terrain::Dirt;
}

void TerrainTesselator::TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess)
{
   csg::Region3 grass, dirt;

   TERRAIN_LOG(7) << "Tesselating Terrain...";
   for (csg::Cube3 const& cube : terrain) {
      switch (cube.GetTag()) {
      case om::Terrain::Grass:
         grass.AddUnique(cube);
         break;
      case om::Terrain::Dirt:
         dirt.AddUnique(cube);
         break;
      default:
         tess.AddUnique(cube);
      }
   }

   AddTerrainTypeToTesselation(grass, terrain, grass_ring_info_, tess);
   AddTerrainTypeToTesselation(dirt, csg::Region3(), dirt_ring_info_, tess);
   TERRAIN_LOG(7) << "Done Tesselating Terrain!";
}

void TerrainTesselator::AddTerrainTypeToTesselation(csg::Region3 const& region, csg::Region3 const& clipper, RingInfo const& ringInfo, csg::Region3& tess)
{
   std::unordered_map<int, csg::Region2> layers;

   for (csg::Cube3 const& cube : region) {
      csg::Point3 const& min = cube.GetMin();
      csg::Point3 const& max = cube.GetMax();
      ASSERT(min.y == max.y - 1); // 1 block thin, pizza box
      layers[min.y].AddUnique(csg::Rect2(
            csg::Point2(min.x, min.z),
            csg::Point2(max.x, max.z)
         ));
   }
   for (auto const& layer : layers) {
      TesselateLayer(layer.second, layer.first, clipper, ringInfo, tess);
   }
}

void TerrainTesselator::TesselateLayer(csg::Region2 const& layer, int height, csg::Region3 const& clipper, RingInfo const& ringInfo, csg::Region3& tess)
{
   csg::Region2 inner = layer;
   csg::EdgeListPtr segments = csg::Region2ToEdgeList(layer, height, clipper);

   for (auto const& layer : ringInfo.rings) {
      TERRAIN_LOG(7) << " Building terrain ring " << height << " " << layer.width;
      csg::Region2 edge = csg::EdgeListToRegion2(segments, layer.width, &inner);      
      for (csg::Rect2 const& rect : edge) {
         csg::Point2 const& min = rect.GetMin();
         csg::Point2 const& max = rect.GetMax();
         csg::Point3 p0(min.x, height, min.y);
         csg::Point3 p1(max.x, height + 1, max.y);
         tess.AddUnique(csg::Cube3(p0, p1, layer.tag));
      }
      segments->Inset(layer.width);
      inner -= edge;
   }
   for (csg::Rect2 const& rect : inner) {
      csg::Point2 const& min = rect.GetMin();
      csg::Point2 const& max = rect.GetMax();
      tess.AddUnique(csg::Cube3(
            csg::Point3(min.x, height, min.y),
            csg::Point3(max.x, height + 1, max.y),
            ringInfo.innerTag
         ));
   }
}
