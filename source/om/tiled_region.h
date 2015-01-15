#ifndef _RADIANT_OM_TILED_REGION_H
#define _RADIANT_OM_TILED_REGION_H

#include "radiant.h"
#include "dm/map.h"
#include "dm/store.h"
#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

template <typename T> class TileMapWrapper;

// Conventions for this class:
//   indexes are always Point3
//   parameters for modifying the region are always [base_type]3f
template <typename T>
class TiledRegion {
public:
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> IndexSet;

   TiledRegion(csg::Point3 const& tile_size, std::shared_ptr<TileMapWrapper<T>> tile_wrapper);

   csg::Point3 GetTileSize() const;
   std::shared_ptr<T> FindTile(csg::Point3 const& index); // returns nulltr if not found
   std::shared_ptr<T> GetTile(csg::Point3 const& index); // creates tile if not found
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

   csg::Region3f IntersectPoint(csg::Point3f const& point);
   csg::Region3f IntersectCube(csg::Cube3f const& cube);
   csg::Region3f IntersectRegion(csg::Region3f const& region);

   bool ContainsPoint(csg::Point3f const& point);

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

// Abstracts operations on tile elements T in a map type
template <typename T>
class TileMapWrapper {
public:
   typedef std::function<void(csg::Region3&)> ModifyRegionFn;

   TileMapWrapper() {}
   virtual void Clear() = 0;
   virtual int NumTiles() = 0;
   virtual std::shared_ptr<T> FindTile(csg::Point3 const& index) = 0; // returns nulltr if not found
   virtual std::shared_ptr<T> GetTile(csg::Point3 const& index) = 0; // creates tile if not found
   virtual void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn) = 0;
   virtual csg::Region3 const& GetTileRegion(std::shared_ptr<T> tile) = 0;
};

// Wrapper for an unordered_map of Region3
class Region3MapWrapper : public TileMapWrapper<csg::Region3> {
public:
   typedef std::unordered_map<csg::Point3, std::shared_ptr<csg::Region3>, csg::Point3::Hash> TileMap;

   Region3MapWrapper(TileMap& tiles);

   void Clear();
   int NumTiles();
   std::shared_ptr<csg::Region3> FindTile(csg::Point3 const& index);
   std::shared_ptr<csg::Region3> GetTile(csg::Point3 const& index);
   void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn);
   csg::Region3 const& GetTileRegion(std::shared_ptr<csg::Region3> tile);

private:
   TileMap& _tiles;
};

// Wrapper for a dm::Map of Region3Boxed
class Region3BoxedMapWrapper : public TileMapWrapper<Region3Boxed> {
public:
   typedef dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash> TileMap;

   Region3BoxedMapWrapper(TileMap& tiles);

   void Clear();
   int NumTiles();
   Region3BoxedPtr FindTile(csg::Point3 const& index);
   Region3BoxedPtr GetTile(csg::Point3 const& index);
   void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn);
   csg::Region3 const& GetTileRegion(Region3BoxedPtr tile);

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
