#ifndef _RADIANT_OM_TILED_REGION_H
#define _RADIANT_OM_TILED_REGION_H

#include <unordered_set>
#include "radiant.h"
#include "dm/map.h"
#include "dm/store.h"
#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

// Abstracts operations on tile elements T in a map type.  This class definition
// exists solely for documentation purposes.  The actual code lives in template specializations
// for each ContainerType.

template <typename ContainerType> 
class TiledRegionAdapter
{
public:
   class TileType;

   void Clear();
   int NumTiles() const;
   std::shared_ptr<TileType> FindTile(csg::Point3 const& index) const;
   std::shared_ptr<TileType> GetTile(csg::Point3 const& index);
   void EachTile(std::function<void(csg::Point3 const&, csg::Region3 const&)> fn) const;
   void ModifyTile(csg::Point3 const& index, std::function<void(csg::Region3 &)> fn);
   csg::Region3 const& GetTileRegion(std::shared_ptr<TileType> tile) const;
};

// Conventions for this class:
//y
//   indicies are always Point3
//   parameters for modifying the region are always [base_type]3f
//   tiles are always some form of integer region
//
template <typename ContainerType, typename TileType>
class TiledRegion
{
public:     // types
   typedef TileType TileType;
   typedef ContainerType ContainerType;
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> IndexSet;
   typedef std::function<void(csg::Region3f const&)> ModifiedCb;
   
public:     // public methods
   TiledRegion(csg::Point3 const& tileSize, ContainerType& tiles);
   ~TiledRegion();

   void SetModifiedCb(ModifiedCb modified_cb);

   bool IsEmpty() const;
   int NumTiles() const;
   int NumCubes() const;
   csg::Point3 GetTileSize() const;

   TileType FindTile(csg::Point3 const& index) const;  // returns nulltr if not found
   TileType GetTile(csg::Point3 const& index);         // creates tile if not found
   void Clear();
   void ClearTile(csg::Point3 const& index);

   IndexSet const& GetChangedSet() const { return _changedSet; }
   void ClearChangedSet();
   void OptimizeChangedTiles(const char* reason);

   void AddPoint(csg::Point3f const& point, int tag = 0);
   void AddCube(csg::Cube3f const& cube);
   void AddRegion(csg::Region3f const& region);

   void SubtractPoint(csg::Point3f const& point);
   void SubtractCube(csg::Cube3f const& cube);
   void SubtractRegion(csg::Region3f const& region);

   csg::Region3f IntersectPoint(csg::Point3f const& point) const;
   csg::Region3f IntersectCube(csg::Cube3f const& cube) const;
   csg::Region3f IntersectRegion(csg::Region3f const& region) const;

   bool Contains(csg::Point3f const& point) const;
   bool IntersectsCube(csg::Cube3f const& cube) const;
   bool IntersectsRegion(csg::Region3f const& region) const;

private:
   void TriggerModified(csg::Region3f const& region);
   void AddToChangedSet(csg::Point3 const& index);
   csg::Cube3f GetTileBounds(csg::Point3 const& index);

private:
   csg::Point3                         _tileSize;
   IndexSet                            _changedSet;
   ModifiedCb                          _modifiedCb;
   TiledRegionAdapter<ContainerType>   _tilemap;
};

// Common template types.
typedef std::unordered_map<csg::Point3, csg::Region3Ptr, csg::Point3::Hash> Region3PtrMap;
typedef TiledRegion<Region3PtrMap, csg::Region3Ptr> Region3Tiled;

typedef dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash> Region3BoxedPtrMap;
typedef TiledRegion<Region3BoxedPtrMap, Region3BoxedPtr> Region3BoxedTiled;

DECLARE_SHARED_POINTER_TYPES(Region3Tiled)
DECLARE_SHARED_POINTER_TYPES(Region3BoxedTiled)

END_RADIANT_OM_NAMESPACE

template <typename ContainerType, typename TileType>
std::ostream& operator<<(std::ostream& out, ::radiant::om::TiledRegion<ContainerType, TileType> const& t);

#endif // _RADIANT_OM_TILED_REGION_H
