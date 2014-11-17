#ifndef _RADIANT_OM_TILED_REGION_H
#define _RADIANT_OM_TILED_REGION_H

#include "radiant.h"
#include "dm/map.h"
#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

template <typename T>
class TileMapWrapper {
public:
   typedef std::function<void(csg::Region3&)> ModifyRegionFn;

   TileMapWrapper() {}
   virtual int NumTiles() = 0;
   virtual std::shared_ptr<T> FindTile(csg::Point3 const& index) = 0; // returns nulltr if not found
   virtual std::shared_ptr<T> GetTile(csg::Point3 const& index) = 0; // creates tile if not found
   virtual void ModifyTile(csg::Point3 const& index, ModifyRegionFn fn) = 0;
   virtual csg::Region3 const& GetTileRegion(std::shared_ptr<T> tile) = 0;
};

template <typename T>
class TiledRegion {
public:
   TiledRegion(csg::Point3 const& tile_size, std::shared_ptr<TileMapWrapper<T>> tile_wrapper);

   csg::Point3 GetTileSize() const;
   std::shared_ptr<T> FindTile(csg::Point3 const& index); // returns nulltr if not found
   std::shared_ptr<T> GetTile(csg::Point3 const& index); // creates tile if not found
   void ClearTile(csg::Point3 const& index);
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

   friend std::ostream& operator<<(std::ostream& out, TiledRegion<T> const& tiled_region);

private:
   csg::Point3 _tile_size;
   std::shared_ptr<TileMapWrapper<T>> _tile_wrapper;
   std::unordered_set<csg::Point3, csg::Point3::Hash> _unoptimized_tiles;
};

#if 0
template <T>
std::ostream& operator<<(std::ostream& out, TiledRegion<T> const& tiled_region);
#else
//template <T>
//std::ostream& operator<<(std::ostream& out, TiledRegion<T> const& tiled_region)
//{
//   out << tiled_region._tiles->Size() << " tiles";
//   return out;
//}
#endif

typedef TiledRegion<Region3Boxed> Region3BoxedPtrTiled;
typedef std::shared_ptr<Region3BoxedPtrTiled> Region3BoxedPtrTiledPtr;

typedef TiledRegion<csg::Region3> Region3PtrTiled;
typedef std::shared_ptr<Region3PtrTiled> Region3PtrTiledPtr;

std::ostream& operator<<(std::ostream& out, Region3BoxedPtrTiled const& tiled_region);
std::ostream& operator<<(std::ostream& out, Region3PtrTiled const& tiled_region);

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_TILED_REGION_H
