#include "radiant.h"
#include "mob.ridl.h"
#include "terrain.ridl.h"
#include "terrain_tesselator.h"
#include "om/entity.h"
#include "om/region.h"
#include "csg/iterators.h"
#include "dm/dm.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TERRAIN_LOG(level)    LOG(simulation.terrain, level)

static const csg::Point3 TILE_SIZE(32, 5, 32);

std::ostream& operator<<(std::ostream& os, Terrain const& o)
{
   return (os << "[Terrain]");
}

void Terrain::LoadFromJson(json::Node const& obj)
{
   bounds_ = csg::Cube3::zero;
}

void Terrain::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}

void Terrain::ConstructObject()
{
   Component::ConstructObject();

   config_file_name_trace_ = TraceConfigFileName("terrain", dm::OBJECT_MODEL_TRACES)
      ->OnModified([this]() {
         res::ResourceManager2::GetInstance().LookupJson(config_file_name_, [&](const json::Node& node) {
            terrainTesselator_.LoadFromJson(node);
         });
      });
}

void Terrain::AddTile(csg::Region3f const& region)
{
   AddTileClipped(region, nullptr);
}

void Terrain::AddTileClipped(csg::Region3f const& region, csg::Rect2f const& clipper)
{
   csg::Rect2 clip = csg::ToInt(clipper);
   AddTileClipped(region, &clip);
}

void Terrain::AddTileClipped(csg::Region3f const& region, csg::Rect2 const* clipper)
{
   csg::Region3 src = csg::ToInt(region); // World space...
   csg::Region3 tesselated = terrainTesselator_.TesselateTerrain(src, clipper);

   csg::PartitionRegionIntoChunksSlow(tesselated, TILE_SIZE, [this](csg::Point3 const& index, csg::Region3 const& region) {
      Region3BoxedPtr tile = GetTiles()->GetTile(index);
      tile->Modify([&region](csg::Region3& cursor) {
         cursor += region;
         cursor.OptimizeByMerge();
      });
      bounds_.Modify([&index](csg::Cube3& cube) {
         cube.Grow(index.Scaled(TILE_SIZE));
         cube.Grow((index + csg::Point3::one).Scaled(TILE_SIZE));
      });
      return false; // don't stop!
   });
}

// if we start streaming in tiles, this will need to return a region instead of a cube
// WARNING: when testing GetBounds().Contains() you must pass in a grid location
csg::Cube3f Terrain::GetBounds() const
{
   return csg::ToFloat(*bounds_);
}

csg::Point3f Terrain::GetPointOnTerrain(csg::Point3f const& location) const
{
   csg::Point3 pt = csg::ToClosestInt(location);
   if (!(*bounds_).Contains(pt)) {
      pt = (*bounds_).GetClosestPoint(pt);
   }

   for (;;) {
      if (!(*bounds_).Contains(pt)) {
         // must have gone outside the top of the box.  no worries!
         break;
      }

      csg::Point3 index, offset;
      csg::GetChunkIndexSlow(pt, TILE_SIZE, index, offset);
      auto i = tiles_.find(index);
      if (i != tiles_.end()) {
         csg::Region3 const& region = i->second->Get();
         if (!region.Contains(pt) && region.Contains(pt - csg::Point3::unitY)) {
            break;
         }
      }
      pt.y++;
   }
   return csg::ToFloat(pt);
}

Region3BoxedTiledPtr Terrain::CreateTileAccessor(Region3BoxedMapWrapper::TileMap& tiles)
{
   std::shared_ptr<Region3BoxedMapWrapper> wrapper = std::make_shared<Region3BoxedMapWrapper>(tiles);
   return std::make_shared<Region3BoxedTiled>(TILE_SIZE, wrapper);
}

Region3BoxedTiledPtr Terrain::GetTiles()
{
   if (!tile_accessor_) {
      tile_accessor_ = CreateTileAccessor(tiles_);
   }
   return tile_accessor_;
}

Region3BoxedTiledPtr Terrain::GetInteriorTiles()
{
   if (!interior_tile_accessor_) {
      interior_tile_accessor_ = CreateTileAccessor(interior_tiles_);
   }
   return interior_tile_accessor_;
}

csg::Point3 const& Terrain::GetTileSize() const
{
   return TILE_SIZE;
}
