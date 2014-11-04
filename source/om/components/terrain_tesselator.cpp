#include "radiant.h"
#include "csg/util.h"
#include "csg/iterators.h"
#include "csg/region_tools.h"
#include "resources/res_manager.h"
#include "terrain.ridl.h"
#include "terrain_tesselator.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

TerrainTesselator::TerrainTesselator()
{
}

void TerrainTesselator::LoadFromJson(json::Node config)
{
   tag_map_.clear();
   ring_infos_.clear();

   json::Node block_types = config.get_node("block_types");

   for (json::Node const& block_type : block_types) {
      tag_map_[block_type.name()] = block_type.get<int>("tag");
   }

   json::Node ring_tesselation = config.get_node("ring_tesselation");

   for (json::Node const& ring_type : ring_tesselation) {
      RingInfo ring_info;

      int base_tag = GetTag(ring_type.name());
      ring_info.innerTag = base_tag;

      for (json::Node const& ring : ring_type) {
         int width = ring.get<int>("width");
         std::string ring_name = ring.get<std::string>("name");
         int tag = GetTag(ring_name);
         ring_info.rings.emplace_back(RingInfo::Ring(width, tag));
      }

      ring_infos_[base_tag] = ring_info;
   }
}

int TerrainTesselator::GetTag(std::string name)
{
   auto i = tag_map_.find(name);
   if (i != tag_map_.end()) {
      return i->second;
   } else {
      throw core::Exception(BUILD_STRING("Unknown terrain block type: " << name));
   }
}

csg::Region3 TerrainTesselator::TesselateTerrain(csg::Region3 const& terrain, csg::Rect2 const* clipper)
{
   csg::Region3 tesselated;
   std::unordered_map<int, std::shared_ptr<csg::Region3>> tesselation_map;

   for (auto i : ring_infos_) {
      tesselation_map[i.first] = std::make_shared<csg::Region3>();
   }

   TERRAIN_LOG(7) << "Tesselating Terrain...";
   for (csg::Cube3 const& cube : csg::EachCube(terrain)) {
      int tag = cube.GetTag();
      auto i = ring_infos_.find(tag);
      if (i == ring_infos_.end()) {
         tesselated.AddUnique(cube);
      } else {
         tesselation_map[tag]->AddUnique(cube);
      }
   }

   for (auto const& i : tesselation_map) {
      csg::Region3& region = *(i.second);
      RingInfo ring_info = ring_infos_[i.first];
      AddTerrainTypeToTesselation(region, clipper, ring_info, tesselated);
   }

   TERRAIN_LOG(7) << "Done Tesselating Terrain!";

   return tesselated;
}

void TerrainTesselator::AddTerrainTypeToTesselation(csg::Region3 const& region, csg::Rect2 const* clipper, RingInfo const& ringInfo, csg::Region3& tess)
{
   std::unordered_map<int, csg::Region2> layers;

   for (csg::Cube3 const& cube : csg::EachCube(region)) {
      csg::Point3 const& min = cube.GetMin();
      csg::Point3 const& max = cube.GetMax();

      // 1 block thin, pizza box
      if (cube.GetSize().y != 1) {
         throw core::Exception("Can only tesselate layers that are 1 voxel thick");
      }

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
      for (csg::Rect2 const& rect : csg::EachCube(edge)) {
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
   for (csg::Rect2 const& rect : csg::EachCube(inner)) {
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