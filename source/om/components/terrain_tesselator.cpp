#include "radiant.h"
#include "csg/util.h"
#include "csg/region_tools.h"
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

csg::Region3 TerrainTesselator::TesselateTerrain(csg::Region3 const& terrain, csg::Rect2 const* clipper)
{
   csg::Region3 tesselated;
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
         tesselated.AddUnique(cube);
      }
   }

   AddTerrainTypeToTesselation(grass, clipper, grass_ring_info_, tesselated);
   AddTerrainTypeToTesselation(dirt, clipper, dirt_ring_info_, tesselated);
   TERRAIN_LOG(7) << "Done Tesselating Terrain!";

   return tesselated;
}

void TerrainTesselator::AddTerrainTypeToTesselation(csg::Region3 const& region, csg::Rect2 const* clipper, RingInfo const& ringInfo, csg::Region3& tess)
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

void TerrainTesselator::TesselateLayer(csg::Region2 const& layer, int height, csg::Rect2 const* clipper, RingInfo const& ringInfo, csg::Region3& tess)
{
   csg::Region2 inner = layer;
   csg::EdgeMap2 edgeMap = csg::RegionTools2().GetEdgeMap(layer);

   // Clip edges before we do anything...
   if (clipper) {
      std::vector<csg::Edge2>& edges = edgeMap.GetEdges();
      uint i = 0, c = edges.size();
      while (i < c) {
         bool filter;
         csg::Edge2 edge = edges[i];
         if (edge.normal.x == 1) {
            filter = edge.max->location.x == clipper->max.x;
         } else if (edge.normal.x == -1) {
            filter = edge.min->location.x == clipper->min.x;
         } else if (edge.normal.y == 1) {
            filter = edge.max->location.y == clipper->max.y;
         } else if (edge.normal.y == -1) {
            filter = edge.min->location.y == clipper->min.y;
         }
         if (filter) {
            csg::Edge2 e = edges[i];
            const_cast<csg::EdgePoint2*>(e.min)->accumulated_normals -= edge.normal;
            const_cast<csg::EdgePoint2*>(e.max)->accumulated_normals -= edge.normal;
            edges[i] = edges[--c];
         } else {
            i++;
         }
      }
      edges.resize(c);
   }

   for (auto const& layer : ringInfo.rings) {
      TERRAIN_LOG(7) << " Building terrain ring " << height << " " << layer.width;
      csg::Region2 edge = CreateRing(edgeMap, layer.width, &inner);      
      for (csg::Rect2 const& rect : edge) {
         csg::Point2 const& min = rect.GetMin();
         csg::Point2 const& max = rect.GetMax();
         csg::Point3 p0(min.x, height, min.y);
         csg::Point3 p1(max.x, height + 1, max.y);
         tess.AddUnique(csg::Cube3(p0, p1, layer.tag));
      }
      inner -= edge;

      // Inset...  May result in invalid edges, which is why we check in EdgeListToRegion2!
      for (csg::EdgePoint2* pt : edgeMap.GetPoints()) {
         pt->location += pt->accumulated_normals * (-layer.width);
      }
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

csg::Region2 TerrainTesselator::CreateRing(csg::EdgeMap2 const& edgeMap, int width, csg::Region2 const* clipper)
{
   csg::Region2 result;

   for (csg::Edge2 const& edge : edgeMap) {
      csg::Point2 const& normal = edge.normal;
      csg::EdgePoint2 const& start = *edge.min;
      csg::EdgePoint2 const& end = *edge.max;

      if (start.location.x <= end.location.x && start.location.y <= end.location.y) {
         csg::Point2 p0, p1;
         csg::Rect2 rect;

         // draw a picture here...
         if (normal.y == -1) {
            p0 = start.location;
            p1 = end.location - (end.accumulated_normals * width);
            rect = csg::Rect2::Construct(p0, p1);
         } else if (normal.y == 1) {
            p0 = start.location - (start.accumulated_normals * width);
            p1 = end.location;
            rect = csg::Rect2::Construct(p0, p1);
         } else if (normal.x == -1) {
            p0 = start.location - (start.accumulated_normals * width);
            p1 = end.location;
            rect = csg::Rect2::Construct(p0, p1);
         } else {
            p0 = start.location;
            p1 = end.location - (end.accumulated_normals * width);
            rect = csg::Rect2::Construct(p0, p1);
         }
         result += rect;
      }
   }
   if (clipper) {
      result &= *clipper;
   }
   return result;
}