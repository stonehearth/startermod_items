#include "radiant.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "terrain_tesselator.h"
#include "om/entity.h"
#include "om/region.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

static float GetPositiveRemainder(float x, float y)
{
   float remainder = std::fmod(x, y);
   if (remainder < 0) {
      remainder += y;
   }
   return remainder;
}

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
   ASSERT(tile_offset.y == 0);
   Region3fBoxedPtr tesselatedRegionBoxed = GetStore().AllocObject<Region3fBoxed>();

   tesselatedRegionBoxed->Modify([this, &region](csg::Region3f& tesselatedRegion) {
         csg::Region3 temp;
         // perform the terrain ring tesselation
         terrainTesselator_.TesselateTerrain(csg::ToInt(region), temp);
         tesselatedRegion = csg::ToFloat(temp);
      });

   float dx = GetPositiveRemainder(tile_offset.x, tile_size_);
   float dz = GetPositiveRemainder(tile_offset.z, tile_size_);

   if (tiles_.IsEmpty()) {
      // the first tile added defines the origin of the coordinate system
      origin_offset_ = csg::Point3f(dx, 0, dz);
   } else {
      // make sure subsequent tiles have origins at the correct offsets
      if (dx != (*origin_offset_).x || dz != (*origin_offset_).z) {
         throw std::invalid_argument("invalid tile_offset");
      }
   }

   // tiles are stored using the location of their 0, 0 coordinate in the world
   tiles_.Add(tile_offset, tesselatedRegionBoxed);

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
   csg::Cube3f result(csg::Cube3f::zero);

   for (auto const& tile : tiles_) {
      result.Grow(GetTileBounds(tile.first));
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
      // faster to test and reject elevation first
      if (cube.GetMax().y > max_y) {
         if (region_local_pt.x >= cube.GetMin().x && region_local_pt.x < cube.GetMax().x &&
             region_local_pt.z >= cube.GetMin().z && region_local_pt.z < cube.GetMax().z) {
            max_y = cube.GetMax().y;
         }
      }
   }
   return csg::Point3f(pt.x, max_y, pt.z);
}

Region3fBoxedPtr Terrain::GetTile(csg::Point3f const& location, csg::Point3f& tile_offset) const
{
   csg::Point3f grid_location = csg::ToFloat(csg::ToClosestInt(location));
   csg::Point3f origin_offset = GetOriginOffset();

   // get the intra-tile offset
   float dx = GetPositiveRemainder(grid_location.x - origin_offset.x, tile_size_);
   float dz = GetPositiveRemainder(grid_location.z - origin_offset.z, tile_size_);

   // remove the intra-tile offset to get to the origin of the tile
   tile_offset.x = grid_location.x - dx;
   tile_offset.y = 0;
   tile_offset.z = grid_location.z - dz;

   auto i = tiles_.find(tile_offset);
   if (i != tiles_.end()) {
      return i->second;
   }

   return nullptr;
}

void Terrain::AddCube(csg::Cube3f const& cube)
{
   AddRegion(cube);
}

void Terrain::SubtractCube(csg::Cube3f const& cube)
{
   SubtractRegion(cube);
}

void Terrain::AddRegion(csg::Region3f const& region)
{
   ApplyRegionToTiles(region, [](csg::Region3f& tile, csg::Region3f& intersection) {
         tile += intersection;
      });
}

void Terrain::SubtractRegion(csg::Region3f const& region)
{
   ApplyRegionToTiles(region, [](csg::Region3f& tile, csg::Region3f& intersection) {
         tile -= intersection;
      });
}

void Terrain::ApplyRegionToTiles(csg::Region3f const& region, ApplyRegionToTileCb const& operation)
{
   csg::Cube3f terrain_bounds = GetBounds();
   Region3fBoxedPtr tile_region_boxed;
   csg::Point3f tile_offset;
   csg::Cube3f tile_bounds;
   csg::Region3f remaining_region;

   // clip the cube by the bounds of the terrain
   // cube must be first operand to preserve the tag
   remaining_region += region & terrain_bounds;

   // apply the remaining region to each tile until it is empty
   while (remaining_region.GetArea() > 0) {
      // grab the next terrain tile using any valid point in the remaining region
      tile_region_boxed = GetTile(remaining_region[0].min, tile_offset);
      tile_bounds = GetTileBounds(tile_offset);

      // get the portion of the remaining region that lies within this tile
      // the tag of the remaining_region is used for the intersection
      csg::Region3f intersection = remaining_region & tile_bounds;

      // remove the intersection from the remaining region
      remaining_region -= intersection;

      // translate the intersection into local coordinates of the tile
      intersection.Translate(-tile_offset);

      // apply the intersection to the terrain tile
      tile_region_boxed->Modify([&intersection, &operation](csg::Region3f& tile_region) {
            operation(tile_region, intersection);
         });
   }
}

csg::Cube3f Terrain::GetTileBounds(csg::Point3f const& tile_offset) const
{
   // y bounds could be infinite, but using +/- tile size for now
   return csg::Cube3f(
         csg::Point3f(tile_offset.x, -tile_size_, tile_offset.z),
         csg::Point3f(tile_offset.x + tile_size_, tile_size_, tile_offset.z + tile_size_)
      );
}
