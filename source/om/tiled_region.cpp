#include "tiled_region.h"
#include "csg/util.h"
#include "csg/iterators.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define TILED_REGION_LOG(level)    LOG(simulation.terrain, level)

// TiledRegion is used for manipulating large regions that need to be spatially subdivided.
template <typename ContainerType, typename TileType>
TiledRegion<ContainerType, TileType>::TiledRegion(csg::Point3 const& tileSize, ContainerType& tiles) :
   _tileSize(tileSize),
   _tilemap(tiles),
   _modifiedCb(nullptr)
{
}

template <typename ContainerType, typename TileType>
TiledRegion<ContainerType, TileType>::~TiledRegion()
{
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::SetModifiedCb(ModifiedCb modified_cb)
{
   _modifiedCb = modified_cb;  
}

template <typename ContainerType, typename TileType>
bool TiledRegion<ContainerType, TileType>::IsEmpty() const
{
   bool empty = NumTiles() == 0;
   return empty;
}

template <typename ContainerType, typename TileType>
csg::Point3 TiledRegion<ContainerType, TileType>::GetTileSize() const
{
   return _tileSize;
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::AddPoint(csg::Point3f const& point3f, int tag)
{
   AddCube(csg::Cube3f(point3f, tag));
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::AddCube(csg::Cube3f const& cube3f)
{
   AddRegion(cube3f);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::AddRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tileSize, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      _tilemap.ModifyTile(index, [&subregion](csg::Region3& cursor) {
         cursor += subregion;
      });
      AddToChangedSet(index);
      return false; // don't stop!
   });

   TriggerModified(region3f);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::SubtractPoint(csg::Point3f const& point3f)
{
   SubtractCube(point3f);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::SubtractCube(csg::Cube3f const& cube3f)
{
   SubtractRegion(cube3f);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::SubtractRegion(csg::Region3f const& region3f)
{
   csg::Region3 region3 = csg::ToInt(region3f);

   csg::PartitionRegionIntoChunksSlow(region3, _tileSize, [this](csg::Point3 const& index, csg::Region3 const& subregion) {
      _tilemap.ModifyTile(index, [&subregion](csg::Region3& cursor) {
         cursor -= subregion;
      });
      AddToChangedSet(index);
      return false; // don't stop!
   });

   TriggerModified(region3f);
}

template <typename ContainerType, typename TileType>
csg::Region3f TiledRegion<ContainerType, TileType>::IntersectPoint(csg::Point3f const& point3f) const
{
   return IntersectCube(point3f);
}

template <typename ContainerType, typename TileType>
csg::Region3f TiledRegion<ContainerType, TileType>::IntersectCube(csg::Cube3f const& cube3f) const
{
   return IntersectRegion(cube3f);
}

// can be further optimized
template <typename ContainerType, typename TileType>
csg::Region3f TiledRegion<ContainerType, TileType>::IntersectRegion(csg::Region3f const& region3f) const
{
   csg::Region3 region3 = csg::ToInt(region3f);   // we expect the region to be simple relative to the stored tiles
   csg::Region3f result;
   csg::Cube3 chunks = csg::GetChunkIndexSlow(csg::ToInt(region3.GetBounds()), _tileSize);

   for (csg::Point3 const& index : csg::EachPoint(chunks)) {
      TileType tile = _tilemap.FindTile(index);
      if (tile) {
         csg::Region3 const& tile_region = _tilemap.GetTileRegion(tile);
         csg::Region3 intersection = tile_region & region3;
         result.AddUnique(csg::ToFloat(intersection));
      }
   }

   return result;
}

template <typename ContainerType, typename TileType>
bool TiledRegion<ContainerType, TileType>::ContainsPoint(csg::Point3f const& point3f) const
{
   csg::Region3f intersection = IntersectPoint(point3f);
   bool contains = !intersection.IsEmpty();
   return contains;
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::Clear()
{
   csg::Region3f cleared_region;

   // doing this to 
   _tilemap.EachTile([this, &cleared_region](csg::Point3 const& index, csg::Region3 const& region) {
         csg::Cube3f tile_bounds = GetTileBounds(index);
         cleared_region.AddUnique(tile_bounds);
      });

   _tilemap.Clear();

   TriggerModified(cleared_region);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::ClearTile(csg::Point3 const& index)
{
   // avoid creating the tile if it doesn't already exist
   TileType tile = FindTile(index);

   if (tile) {
      _tilemap.ModifyTile(index, [](csg::Region3& cursor) {
         cursor.Clear();
      });

      csg::Cube3f tile_bounds = GetTileBounds(index);
      TriggerModified(csg::Region3f(tile_bounds));
   }
}

// returns nulltr if not found
template <typename ContainerType, typename TileType>
TileType TiledRegion<ContainerType, TileType>::FindTile(csg::Point3 const& index) const
{
   TileType tile = _tilemap.FindTile(index);
   return tile;
}

// creates tile if not found
template <typename ContainerType, typename TileType>
TileType TiledRegion<ContainerType, TileType>::GetTile(csg::Point3 const& index)
{
   TileType tile = _tilemap.GetTile(index);
   return tile;
}

template <typename ContainerType, typename TileType>
int TiledRegion<ContainerType, TileType>::NumTiles() const
{
   return _tilemap.NumTiles();
}

template <typename ContainerType, typename TileType>
int TiledRegion<ContainerType, TileType>::NumCubes() const
{
   int total = 0;
   _tilemap.EachTile([&total](csg::Point3 const& index, csg::Region3 const& region) {
      total += region.GetCubeCount();
   });
   return total;
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::AddToChangedSet(csg::Point3 const& index)
{
   _changedSet.insert(index);
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::ClearChangedSet()
{
   _changedSet.clear();
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::OptimizeChangedTiles()
{
   for (csg::Point3 index : _changedSet) {
      _tilemap.ModifyTile(index, [](csg::Region3& cursor) {
         cursor.OptimizeByMerge();
      });
   }
}

template <typename ContainerType, typename TileType>
void TiledRegion<ContainerType, TileType>::TriggerModified(csg::Region3f const& region)
{
   if (region.IsEmpty()) {
      return;  
   }
   if (_modifiedCb) {
      _modifiedCb(region);
   }
}

template <typename ContainerType, typename TileType>
csg::Cube3f TiledRegion<ContainerType, TileType>::GetTileBounds(csg::Point3 const& index)
{
   csg::Point3f min = csg::ToFloat(index * _tileSize);
   csg::Point3f max = csg::ToFloat((index + csg::Point3::one) * _tileSize);
   return csg::Cube3f(min, max);
}

// Region3PtrMap 

template <>
class TiledRegionAdapter<Region3PtrMap>
{
public:     // types
   typedef csg::Region3 RegionType;

public:
   TiledRegionAdapter(Region3PtrMap& container) : _container(container) { }
   ~TiledRegionAdapter() { }

   void Clear() {
      for (auto const& entry : _container) {
         entry.second->Clear();
      }
   }

   int NumTiles() const {
      return static_cast<int>(_container.size());
   }

   csg::Region3Ptr FindTile(csg::Point3 const& index) const {
      csg::Region3Ptr tile;
      auto i = _container.find(index);
      if (i != _container.end()) {
         tile = i->second;
      }
      return tile;
   }

   csg::Region3Ptr GetTile(csg::Point3 const& index) {
      csg::Region3Ptr tile = FindTile(index);
      if (!tile) {
         tile = std::make_shared<csg::Region3>();
         _container[index] = tile;
      }
      return tile;
   }

   csg::Region3 const& GetTileRegion(csg::Region3Ptr tile) const {
      if (!tile) {
         ASSERT(false);
         throw core::Exception("null tile");
      }
      return *tile;
   }

   void EachTile(std::function<void(csg::Point3 const&, csg::Region3 const&)> fn) const {
      for (auto const& entry : _container) {
         fn(entry.first, *entry.second);
      }
   }

   void ModifyTile(csg::Point3 const& index, std::function<void(csg::Region3 &)> fn) {
      csg::Region3Ptr tile = GetTile(index);
      fn(*tile);
   }

private:
   Region3PtrMap&  _container;
};

// Region3BoxedPtrMap 

template <>
class TiledRegionAdapter<Region3BoxedPtrMap>
{
public:     // types
   typedef om::Region3Boxed RegionType;

public:
   TiledRegionAdapter(Region3BoxedPtrMap& container) : _container(container) { }
   ~TiledRegionAdapter() { }

   void Clear() {
      for (auto const& entry : _container) {
         entry.second->Modify([](csg::Region3& cursor) {
            cursor.Clear();
         });
      }
   }

   int NumTiles() const {
      return _container.Size();
   }

   Region3BoxedPtr FindTile(csg::Point3 const& index) const {
      Region3BoxedPtr tile;
      auto i = _container.find(index);
      if (i != _container.end()) {
         tile = i->second;
      }
      return tile;
   }

   Region3BoxedPtr GetTile(csg::Point3 const& index) {
      Region3BoxedPtr tile = FindTile(index);
      if (!tile) {
         tile = _container.GetStore().AllocObject<Region3Boxed>();
         _container.Add(index, tile);
      }
      return tile;
   }

   void EachTile(std::function<void(csg::Point3 const&, csg::Region3 const&)> fn) const {
      for (auto const& entry : _container) {
         fn(entry.first, entry.second->Get());
      }
   }

   void ModifyTile(csg::Point3 const& index, std::function<void(csg::Region3 &)> fn) {
      GetTile(index)->Modify(fn);
   }

   csg::Region3 const& GetTileRegion(Region3BoxedPtr tile) const {
      if (!tile) {
         ASSERT(false);
         throw core::Exception("null tile");
      }
      return tile->Get();
   }

private:
   Region3BoxedPtrMap&   _container;
};

// instantiate the common template types
template Region3Tiled;
template Region3BoxedTiled;
