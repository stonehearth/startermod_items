#ifndef _RADIANT_OM_TILED_REGION_H
#define _RADIANT_OM_TILED_REGION_H

#include "radiant.h"
#include "dm/map.h"
#include "dm/store.h"
#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

// Abstracts operations on tile elements T in a map type
template <typename T>
class TileMapWrapper {
public:
   typedef std::function<void(csg::Region3&)> ModifyRegionFn;
   typedef std::function<void(csg::Point3 const&, csg::Region3 const&)> EachTileCb;

   TileMapWrapper() {}
   virtual void Clear() = 0;
   virtual int NumTiles() const = 0;
   virtual std::shared_ptr<T> FindTile(csg::Point3 const& index) const = 0; // returns nulltr if not found
   virtual std::shared_ptr<T> GetTile(csg::Point3 const& index) = 0; // creates tile if not found
   virtual void EachTile(EachTileCb fn) const = 0;
   virtual void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn) = 0;
   virtual csg::Region3 const& GetTileRegion(std::shared_ptr<T> tile) const = 0;
};

// Conventions for this class:
//   indicies are always Point3
//   parameters for modifying the region are always [base_type]3f
template <typename T>
class TiledRegion {
public:
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> IndexSet;

   TiledRegion(csg::Point3 const& tile_size, std::shared_ptr<TileMapWrapper<T>> tile_wrapper);

   bool IsEmpty() const;
   csg::Point3 GetTileSize() const;
   std::shared_ptr<T> FindTile(csg::Point3 const& index) const; // returns nulltr if not found
   std::shared_ptr<T> GetTile(csg::Point3 const& index); // creates tile if not found
   void EachTile(typename TileMapWrapper<T>::EachTileCb fn) const;
   void Clear();
   void ClearTile(csg::Point3 const& index);
   IndexSet const& GetChangedSet() const { return _changed_set; }
   void ClearChangedSet();
   void OptimizeChangedTiles();

   void AddPoint(csg::Point3f const& point, int tag = 0);
   void AddCube(csg::Cube3f const& cube);
   void AddRegion(csg::Region3f const& region);

   void SubtractPoint(csg::Point3f const& point);
   void SubtractCube(csg::Cube3f const& cube);
   void SubtractRegion(csg::Region3f const& region);

   csg::Region3f IntersectPoint(csg::Point3f const& point) const;
   csg::Region3f IntersectCube(csg::Cube3f const& cube) const;
   csg::Region3f IntersectRegion(csg::Region3f const& region) const;

   bool ContainsPoint(csg::Point3f const& point) const;

   int NumTiles() const;
   int NumCubes() const;

   // keeping this inline as it gets messy otherwise
   friend std::ostream& operator<<(std::ostream& out, TiledRegion const& tiled_region) {
      out << tiled_region._tile_wrapper->NumTiles() << " tiles";
      return out;
   }

private:
   void AddToChangedSet(csg::Point3 const& index);

   csg::Point3 _tile_size;
   std::shared_ptr<TileMapWrapper<T>> _tile_wrapper;
   IndexSet _changed_set;
};

// Wrapper for an unordered_map of Region3
class Region3MapWrapper : public TileMapWrapper<csg::Region3> {
public:
   typedef std::unordered_map<csg::Point3, std::shared_ptr<csg::Region3>, csg::Point3::Hash> TileMap;

   Region3MapWrapper(TileMap& tiles);

   void Clear();
   int NumTiles() const;
   std::shared_ptr<csg::Region3> FindTile(csg::Point3 const& index) const;
   std::shared_ptr<csg::Region3> GetTile(csg::Point3 const& index);
   void EachTile(EachTileCb fn) const;
   void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn);
   csg::Region3 const& GetTileRegion(std::shared_ptr<csg::Region3> tile) const;

private:
   TileMap& _tiles;
};

// Wrapper for a dm::Map of Region3Boxed
class Region3BoxedMapWrapper : public TileMapWrapper<Region3Boxed> {
public:
   typedef dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash> TileMap;

   Region3BoxedMapWrapper(TileMap& tiles);

   void Clear();
   int NumTiles() const;
   Region3BoxedPtr FindTile(csg::Point3 const& index) const;
   Region3BoxedPtr GetTile(csg::Point3 const& index);
   void EachTile(EachTileCb fn) const;
   void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn);
   csg::Region3 const& GetTileRegion(Region3BoxedPtr tile) const;

private:
   TileMap& _tiles;
   dm::Store& _store;
};

// typedef the common template types
typedef TiledRegion<csg::Region3> Region3Tiled;
typedef TiledRegion<Region3Boxed> Region3BoxedTiled;

// and their shared pointers
typedef std::shared_ptr<Region3Tiled> Region3TiledPtr;
typedef std::shared_ptr<Region3BoxedTiled> Region3BoxedTiledPtr;

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_TILED_REGION_H
