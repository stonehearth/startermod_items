#include "tiled_region.h"
#include "csg/util.h"
#include "csg/iterators.h"

using namespace ::radiant;
using namespace ::radiant::om;

// TiledRegion is used for manipulating large regions that need to be spatially subdivided.
template <typename T>
TiledRegion<T>::TiledRegion(csg::Point3 const& tile_size, std::shared_ptr<TileMapWrapper<T>> tile_wrapper) :
   _tile_size(tile_size),
   _tile_wrapper(tile_wrapper)
{
}

template <typename T>
csg::Point3 TiledRegion<T>::GetTileSize() const
{
   return _tile_size;
}

template <typename T>
void TiledRegion<T>::AddPoint(csg::Point3f const& point3f, int tag)
{
   AddCube(csg::Cube3f(point3f, tag));
}

template <typename T>
void TiledRegion<T>::AddCube(csg::Cube3f const& cube3f)
{
   AddRegion(cube3f);
}

template <typename T>
void TiledRegion<T>::AddRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tile_size, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      _tile_wrapper->ModifyTile(index, [&subregion](csg::Region3& cursor) {
         cursor += subregion;
      });
      _unoptimized_tiles.insert(index);
      return false; // don't stop!
   });
}

template <typename T>
void TiledRegion<T>::SubtractPoint(csg::Point3f const& point3f)
{
   SubtractCube(point3f);
}

template <typename T>
void TiledRegion<T>::SubtractCube(csg::Cube3f const& cube3f)
{
   SubtractRegion(cube3f);
}

template <typename T>
void TiledRegion<T>::SubtractRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tile_size, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      _tile_wrapper->ModifyTile(index, [&subregion](csg::Region3& cursor) {
         cursor -= subregion;
      });
      _unoptimized_tiles.insert(index);
      return false; // don't stop!
   });
}

template <typename T>
csg::Region3f TiledRegion<T>::IntersectPoint(csg::Point3f const& point3f)
{
   return IntersectCube(point3f);
}

template <typename T>
csg::Region3f TiledRegion<T>::IntersectCube(csg::Cube3f const& cube3f)
{
   return IntersectRegion(cube3f);
}

// can be further optimized
template <typename T>
csg::Region3f TiledRegion<T>::IntersectRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);   // we expect the region to be simple relative to the stored tiles
   csg::Region3f result;
   csg::Cube3 chunks = csg::GetChunkIndexSlow(csg::ToInt(region3.GetBounds()), _tile_size);

   for (csg::Point3 const& index : csg::EachPoint(chunks)) {
      std::shared_ptr<T> tile = _tile_wrapper->FindTile(index);
      if (tile) {
         csg::Region3 const& tile_region = _tile_wrapper->GetTileRegion(tile);
         csg::Region3 intersection = tile_region & region3;
         result.AddUnique(csg::ToFloat(intersection));
      }
   }

   return result;
}

// The unoptimized set is currently lost on save
template <typename T>
void TiledRegion<T>::OptimizeChangedTiles()
{
   for (csg::Point3 index : _unoptimized_tiles) {
      _tile_wrapper->ModifyTile(index, [](csg::Region3& cursor) {
         cursor.OptimizeByMerge();
      });
   }
   _unoptimized_tiles.clear();
}

template <typename T>
void TiledRegion<T>::ClearTile(csg::Point3 const& index)
{
   // avoid creating the tile if it doesn't already exist
   std::shared_ptr<T> tile = FindTile(index);

   if (tile) {
      _tile_wrapper->ModifyTile(index, [](csg::Region3& cursor) {
         cursor.Clear();
      });
   }
}

// returns nulltr if not found
template <typename T>
std::shared_ptr<T> TiledRegion<T>::FindTile(csg::Point3 const& index)
{
   std::shared_ptr<T> tile = _tile_wrapper->FindTile(index);
   return tile;
}

// creates tile if not found
template <typename T>
std::shared_ptr<T> TiledRegion<T>::GetTile(csg::Point3 const& index)
{
   std::shared_ptr<T> tile = _tile_wrapper->GetTile(index);
   return tile;
}

#if 0
template <typename T>
std::ostream& om::operator<<(std::ostream& out, TiledRegion<T> const& tiled_region)
{
   out << tiled_region._tile_wrapper->NumTiles() << " tiles";
   return out;
}
template std::ostream& om::operator<<(std::ostream& out, Region3BoxedPtrTiled const& tiled_region);
#endif

std::ostream& om::operator<<(std::ostream& out, Region3BoxedPtrTiled const& tiled_region)
{
   out << tiled_region._tile_wrapper->NumTiles() << " tiles";
   return out;
}

std::ostream& om::operator<<(std::ostream& out, Region3PtrTiled const& tiled_region)
{
   out << tiled_region._tile_wrapper->NumTiles() << " tiles";
   return out;
}

template Region3BoxedPtrTiled;
template Region3PtrTiled;
