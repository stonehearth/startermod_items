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
bool TiledRegion<T>::IsEmpty() const
{
   bool empty = NumTiles() == 0;
   return empty;
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
      AddToChangedSet(index);
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
      AddToChangedSet(index);
      return false; // don't stop!
   });
}

template <typename T>
csg::Region3f TiledRegion<T>::IntersectPoint(csg::Point3f const& point3f) const
{
   return IntersectCube(point3f);
}

template <typename T>
csg::Region3f TiledRegion<T>::IntersectCube(csg::Cube3f const& cube3f) const
{
   return IntersectRegion(cube3f);
}

// can be further optimized
template <typename T>
csg::Region3f TiledRegion<T>::IntersectRegion(csg::Region3f const& region3f) const
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

template <typename T>
bool TiledRegion<T>::ContainsPoint(csg::Point3f const& point3f) const
{
   csg::Region3f intersection = IntersectPoint(point3f);
   bool contains = !intersection.IsEmpty();
   return contains;
}

template <typename T>
void TiledRegion<T>::Clear()
{
   _tile_wrapper->Clear();
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
std::shared_ptr<T> TiledRegion<T>::FindTile(csg::Point3 const& index) const
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

template <typename T>
void TiledRegion<T>::EachTile(typename TileMapWrapper<T>::EachTileCb fn) const
{
   _tile_wrapper->EachTile(fn);
}

template <typename T>
int TiledRegion<T>::NumTiles() const
{
   return _tile_wrapper->NumTiles();
}

template <typename T>
int TiledRegion<T>::NumCubes() const
{
   int total = 0;
   EachTile([&total](csg::Point3 const& index, csg::Region3 const& region) {
      total += region.GetCubeCount();
   });
   return total;
}

template <typename T>
void TiledRegion<T>::AddToChangedSet(csg::Point3 const& index)
{
   _changed_set.insert(index);
}

template <typename T>
void TiledRegion<T>::ClearChangedSet()
{
   _changed_set.clear();
}

template <typename T>
void TiledRegion<T>::OptimizeChangedTiles()
{
   for (csg::Point3 index : _changed_set) {
      _tile_wrapper->ModifyTile(index, [](csg::Region3& cursor) {
         cursor.OptimizeByMerge();
      });
   }
}

//------------------
// Region3MapWrapper
//------------------

Region3MapWrapper::Region3MapWrapper(TileMap& tiles) :
   _tiles(tiles)
{}


void Region3MapWrapper::Clear()
{
   for (auto const& entry : _tiles) {
      entry.second->Clear();
   }
}

int Region3MapWrapper::NumTiles() const
{
   return (int)_tiles.size();
}

std::shared_ptr<csg::Region3> Region3MapWrapper::FindTile(csg::Point3 const& index) const
{
   std::shared_ptr<csg::Region3> tile = nullptr;
   auto i = _tiles.find(index);
   if (i != _tiles.end()) {
      tile = i->second;
   }
   return tile;
}

std::shared_ptr<csg::Region3> Region3MapWrapper::GetTile(csg::Point3 const& index)
{
   std::shared_ptr<csg::Region3> tile = FindTile(index);
   if (!tile) {
      tile = std::make_shared<csg::Region3>();
      _tiles[index] = tile;
   }
   return tile;
}

void Region3MapWrapper::EachTile(EachTileCb fn) const
{
   for (auto const& entry : _tiles) {
      csg::Point3 const& index = entry.first;
      csg::Region3 const& region = GetTileRegion(entry.second);
      fn(index, region);
   }
}

void Region3MapWrapper::ModifyTile(csg::Point3 const& index, ModifyRegionFn fn)
{
   std::shared_ptr<csg::Region3> tile = GetTile(index);
   fn(*tile);
}

csg::Region3 const& Region3MapWrapper::GetTileRegion(std::shared_ptr<csg::Region3> tile) const
{
   if (!tile) {
      ASSERT(false);
      throw core::Exception("null tile");
   }
   return *tile;
}

//-----------------------
// Region3BoxedMapWrapper
//-----------------------

Region3BoxedMapWrapper::Region3BoxedMapWrapper(TileMap& tiles) :
   _tiles(tiles),
   _store(tiles.GetStore())
{}

void Region3BoxedMapWrapper::Clear()
{
   for (auto const& entry : _tiles) {
      entry.second->Modify([](csg::Region3& cursor) {
         cursor.Clear();
      });
   }
}

int Region3BoxedMapWrapper::NumTiles() const
{
   return _tiles.Size();
}

Region3BoxedPtr Region3BoxedMapWrapper::FindTile(csg::Point3 const& index) const
{
   Region3BoxedPtr tile = nullptr;
   auto i = _tiles.find(index);
   if (i != _tiles.end()) {
      tile = i->second;
   }
   return tile;
}

Region3BoxedPtr Region3BoxedMapWrapper::GetTile(csg::Point3 const& index)
{
   Region3BoxedPtr tile = FindTile(index);
   if (!tile) {
      tile = _store.AllocObject<Region3Boxed>();
      _tiles.Add(index, tile);
   }
   return tile;
}

void Region3BoxedMapWrapper::EachTile(EachTileCb fn) const
{
   for (auto const& entry : _tiles) {
      csg::Point3 const& index = entry.first;
      csg::Region3 const& region = GetTileRegion(entry.second);
      fn(index, region);
   }
}

void Region3BoxedMapWrapper::ModifyTile(csg::Point3 const& index, ModifyRegionFn fn)
{
   Region3BoxedPtr tile = GetTile(index);
   tile->Modify(fn);
}

csg::Region3 const& Region3BoxedMapWrapper::GetTileRegion(Region3BoxedPtr tile) const
{
   if (!tile) {
      ASSERT(false);
      throw core::Exception("null tile");
   }
   return tile->Get();
}

// instantiate the common template types
template Region3Tiled;
template Region3BoxedTiled;
