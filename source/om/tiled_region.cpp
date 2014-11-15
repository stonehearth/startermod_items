#include "tiled_region.h"
#include "csg/util.h"
#include "csg/iterators.h"

using namespace ::radiant;
using namespace ::radiant::om;

// TiledRegion is used for manipulating large regions that need to be spatially subdivided.
TiledRegion::TiledRegion() :
   _tiles(nullptr),
   _tile_size(csg::Point3::zero),
   _store(nullptr)
{
}

TiledRegion::TiledRegion(TileMap3& tiles, csg::Point3 const& tile_size, dm::Store& store)
{
   Initialize(tiles, tile_size, store);
}

void TiledRegion::Initialize(TileMap3& tiles, csg::Point3 const& tile_size, dm::Store& store)
{
   _tiles = &tiles;
   _tile_size = tile_size;
   _store = &store;
}

csg::Point3 TiledRegion::GetTileSize() const
{
   return _tile_size;
}

void TiledRegion::AddPoint(csg::Point3f const& point3f, int tag)
{
   AddCube(csg::Cube3f(point3f, tag));
}

void TiledRegion::AddCube(csg::Cube3f const& cube3f)
{
   AddRegion(cube3f);
}

void TiledRegion::AddRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tile_size, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      Region3BoxedPtr tile = GetTile(index);
      tile->Modify([&subregion](csg::Region3& cursor) {
         cursor += subregion;
      });
      _unoptimized_tiles.insert(index);
      return false; // don't stop!
   });
}

csg::Region3f TiledRegion::IntersectPoint(csg::Point3f const& point3f)
{
   return IntersectCube(point3f);
}

csg::Region3f TiledRegion::IntersectCube(csg::Cube3f const& cube3f)
{
   return IntersectRegion(cube3f);
}

// can be further optimized
csg::Region3f TiledRegion::IntersectRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);   // we expect the region to be simple relative to the stored tiles
   csg::Region3f result;
   csg::Cube3 chunks = csg::GetChunkIndexSlow(csg::ToInt(region3.GetBounds()), _tile_size);

   for (csg::Point3 const& index : csg::EachPoint(chunks)) {
      auto i = _tiles->find(index);
      if (i != _tiles->end()) {
         Region3BoxedPtr tile = i->second;
         csg::Region3 intersection = tile->Get() & region3;
         result.AddUnique(csg::ToFloat(intersection));
      }
   }

   return result;
}

void TiledRegion::SubtractPoint(csg::Point3f const& point3f)
{
   SubtractCube(point3f);
}

void TiledRegion::SubtractCube(csg::Cube3f const& cube3f)
{
   SubtractRegion(cube3f);
}

void TiledRegion::SubtractRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tile_size, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      Region3BoxedPtr tile = GetTile(index);
      tile->Modify([&subregion](csg::Region3& cursor) {
         cursor -= subregion;
      });
      _unoptimized_tiles.insert(index);
      return false; // don't stop!
   });
}

// The unoptimized set is currently lost on save
void TiledRegion::OptimizeChangedTiles()
{
   for (csg::Point3 index : _unoptimized_tiles) {
      Region3BoxedPtr tile = GetTile(index);
      tile->Modify([](csg::Region3& cursor) {
         cursor.OptimizeByMerge();
      });
   }
   _unoptimized_tiles.clear();
}

Region3BoxedPtr TiledRegion::GetTile(csg::Point3 const& index, bool add_if_not_found)
{
   Region3BoxedPtr tile = nullptr;
   auto i = _tiles->find(index);
   if (i != _tiles->end()) {
      tile = i->second;
   } else {
      if (add_if_not_found) {
         tile = _store->AllocObject<Region3Boxed>();
         _tiles->Add(index, tile);
      }
   }
   return tile;
}

void TiledRegion::ClearTile(csg::Point3 const& index)
{
   Region3BoxedPtr region3boxed = GetTile(index, false);

   if (region3boxed) {
      region3boxed->Modify([](csg::Region3& cursor) {
         cursor.Clear();
      });
   }
}

std::ostream& om::operator<<(std::ostream& out, TiledRegion const& tiled_region)
{
   out << tiled_region._tiles->Size() << " tiles";
   return out;
}
