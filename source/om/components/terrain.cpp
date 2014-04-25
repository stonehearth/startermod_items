#include "pch.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
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
   cached_bounds_ = csg::Cube3::zero;
}

void Terrain::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}

void Terrain::AddTile(csg::Point3 const& tile_offset, Region3BoxedPtr region3)
{
   // tiles are stored using the location of their 0, 0 coordinate in the world
   tiles_.Add(tile_offset, region3);

   cached_bounds_ = CalculateBounds();
}

bool Terrain::InBounds(csg::Point3 const& location) const
{
   bool inBounds = GetBounds().Contains(location);
   return inBounds;
}

// if we start streaming in tiles, this will need to return a region instead of a cube
csg::Cube3 Terrain::GetBounds() const
{
   return cached_bounds_;
}

csg::Cube3 Terrain::CalculateBounds() const
{
   int const tileSize = GetTileSize();
   csg::Point3 const cubeSize(tileSize, tileSize, tileSize);
   csg::Cube3 result = csg::Cube3::zero;

   if (tiles_.Size() > 0) {
      auto const& firstTile = tiles_.begin();
      result = csg::Cube3(firstTile->first, firstTile->first + cubeSize);
      for (auto const& tile : tiles_) {
         result.Grow(tile.first);
         result.Grow(tile.first + cubeSize);
      }
   }

   return result;
}

void Terrain::PlaceEntity(EntityRef e, csg::Point3 const& location)
{
   csg::Point3 placed_location;
   auto entity = e.lock();
   if (entity) {
      csg::Cube3 bounds = GetBounds();

      if (bounds.Contains(location)) {
         placed_location = location;
      } else {
         TERRAIN_LOG(5) << "Cannot place entity at " << location << " because it is not in the world. Moving to closest world location.";
         placed_location = bounds.GetClosestPoint(location);
      }

      placed_location.y = GetHeight(placed_location.x, placed_location.z);

      if (placed_location.y != INT_MIN) {
         auto mob = entity->GetComponent<Mob>();
         if (mob) {
            mob->MoveToGridAligned(placed_location);
         }
      }
   }
}

// returns the maximum height at (x,z)
// throws exception if (x,z) is not in the world
// returns INT_MIN if no cube intersects the (x,z) column
int Terrain::GetHeight(int x, int z) const
{
   int max_y = INT_MIN;
   csg::Point3 const location3 = csg::Point3(x, 0, z);
   csg::Point3 tile_offset;
   Region3BoxedPtr region_ptr = GetTile(location3, tile_offset);
   csg::Point3 const& region_local_pt = location3 - tile_offset;

   if (!region_ptr) {
      throw std::invalid_argument(BUILD_STRING("point " << location3 << " is not in world"));
   }
   csg::Region3 const& region = region_ptr->Get();

   // O(n) search - consider optimizing
   for (csg::Cube3 const& cube : region) {
      if (region_local_pt.x >= cube.GetMin().x && region_local_pt.x < cube.GetMax().x &&
            region_local_pt.z >= cube.GetMin().z && region_local_pt.z < cube.GetMax().z) {
         max_y = std::max(cube.GetMax().y, max_y);
      }
   }
   return max_y;
}

Region3BoxedPtr Terrain::GetTile(csg::Point3 const& location, csg::Point3& tile_offset) const
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
