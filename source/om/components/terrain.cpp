#include "pch.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "om/entity.h"
#include "om/region.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Terrain const& o)
{
   return (os << "[Terrain]");
}

void Terrain::CreateNew()
{
}

void Terrain::ExtendObject(json::Node const& obj)
{
}

void Terrain::AddTile(csg::Point3 const& tile_offset, Region3BoxedPtr region3)
{
   // tiles are stored using the location of their 0, 0 coordinate in the world
   tiles_.Add(tile_offset, region3);
}

csg::Cube3 Terrain::GetBounds()
{
   csg::Cube3 result = csg::Cube3::zero;

   if (tiles_.Size() > 0) {
      const auto& firstTile = tiles_.begin();
      result = csg::Cube3(firstTile->first, firstTile->first + csg::Point3(GetTileSize(), GetTileSize(), GetTileSize()));
      for (const auto& tile : tiles_) {
         result.Grow(tile.first);
         result.Grow(tile.first + csg::Point3(GetTileSize(), GetTileSize(), GetTileSize()));
      }
   }
   return result;
}

void Terrain::PlaceEntity(EntityRef e, const csg::Point3& location)
{
   auto entity = e.lock();
   if (entity) {
      int height = GetHeight(location);

      if (height != INT_MIN) {
         auto mob = entity->GetComponent<Mob>();
         if (mob) {
            mob->MoveToGridAligned(csg::Point3(location.x, height, location.z));
         }
      }
   }
}

// returns the maximum height at (x,z)
// returns INT_MIN if no cube is found over (x,z)
int Terrain::GetHeight(csg::Point3 const& location)
{
   int max_y = INT_MIN;
   csg::Point3 tile_offset;
   Region3BoxedPtr region_ptr = GetTile(location, tile_offset);
   csg::Point3 const& region_local_pt = location - tile_offset;

   if (!region_ptr) {
      throw std::invalid_argument(BUILD_STRING("point " << location << " is not in world"));
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

Region3BoxedPtr Terrain::GetTile(csg::Point3 const& location, csg::Point3& tile_offset)
{
   // O(n) search - consider optimizing
   for (auto& entry : tiles_) {
      tile_offset = entry.first;
      if (location.x >= tile_offset.x && location.x < tile_offset.x + tile_size_ &&
          location.z >= tile_offset.z && location.z < tile_offset.z + tile_size_) {
         return entry.second;
      }
   }
   return nullptr;
}
