#include "radiant.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "terrain_tesselator.h"
#include "om/entity.h"
#include "om/region.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

std::ostream& operator<<(std::ostream& os, Terrain const& o)
{
   return (os << "[Terrain]");
}

void Terrain::LoadFromJson(json::Node const& obj)
{
   cached_bounds_ = csg::Cube3f::zero;
}

void Terrain::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}

void Terrain::AddTile(csg::Point3f const& tile_offset, csg::Region3f const& region)
{
   Region3fBoxedPtr boxedTesselatedRegion = GetStore().AllocObject<Region3fBoxed>();

   boxedTesselatedRegion->Modify([this, &region](csg::Region3f& tesselatedRegion) {
         csg::Region3 temp;
         terrainTesselator_.TesselateTerrain(csg::ToInt(region), temp);
         tesselatedRegion = csg::ToFloat(temp);
      });

   // tiles are stored using the location of their 0, 0 coordinate in the world
   tiles_.Add(tile_offset, boxedTesselatedRegion);

   cached_bounds_ = CalculateBounds();
}

bool Terrain::InBounds(csg::Point3f const& location) const
{
   bool inBounds = GetBounds().Contains(location);
   return inBounds;
}

// if we start streaming in tiles, this will need to return a region instead of a cube
csg::Cube3f Terrain::GetBounds() const
{
   return cached_bounds_;
}

csg::Cube3f Terrain::CalculateBounds() const
{
   float tileSize = GetTileSize();
   csg::Point3f cubeSize(tileSize, tileSize, tileSize);
   csg::Cube3f result(csg::Cube3f::zero);

   if (tiles_.Size() > 0) {
      auto const& firstTile = tiles_.begin();
      result = csg::Cube3f(firstTile->first, firstTile->first + cubeSize);
      for (auto const& tile : tiles_) {
         result.Grow(tile.first);
         result.Grow(tile.first + cubeSize);
      }
   }

   return result;
}

csg::Point3f Terrain::GetPointOnTerrain(csg::Point3f const& location) const
{
   csg::Cube3f bounds = GetBounds();
   csg::Point3f pt;

   if (bounds.Contains(location)) {
      pt = location;
   } else {
      pt = bounds.GetClosestPoint(location);
   }

   float max_y = FLT_MIN;
   csg::Point3f tile_offset;
   Region3fBoxedPtr region_ptr = GetTile(pt, tile_offset);
   csg::Point3f const& region_local_pt = pt - tile_offset;

   if (!region_ptr) {
      throw std::invalid_argument(BUILD_STRING("point " << pt << " is not in world (bounds:" << bounds << ")"));
   }
   csg::Region3f const& region = region_ptr->Get();

   // O(n) search - consider optimizing
   for (csg::Cube3f const& cube : region) {
      if (region_local_pt.x >= cube.GetMin().x && region_local_pt.x < cube.GetMax().x &&
          region_local_pt.z >= cube.GetMin().z && region_local_pt.z < cube.GetMax().z) {
         max_y = std::max(cube.GetMax().y, max_y);
      }
   }
   return csg::Point3f(pt.x, max_y, pt.z);
}

Region3fBoxedPtr Terrain::GetTile(csg::Point3f const& location, csg::Point3f& tile_offset) const
{
   // O(n) search - consider optimizing
   for (auto const& entry : tiles_) {
      tile_offset = entry.first;
      if (location.x >= tile_offset.x && location.x < tile_offset.x + tile_size_ &&
          location.z >= tile_offset.z && location.z < tile_offset.z + tile_size_) {
         return entry.second;
      }
   }
   return nullptr;
}
