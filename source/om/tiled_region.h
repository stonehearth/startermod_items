#ifndef _RADIANT_OM_TILED_REGION_H
#define _RADIANT_OM_TILED_REGION_H

#include "radiant.h"
#include "dm/map.h"
#include "dm/store.h"
#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

class TiledRegion;
typedef std::shared_ptr<TiledRegion> TiledRegionPtr;
typedef dm::Map<csg::Point3, Region3BoxedPtr, csg::Point3::Hash> TileMap3;

class TiledRegion {
public:
   TiledRegion();
   TiledRegion(TileMap3& tiles, csg::Point3 const& tile_size, dm::Store& store);
   void Initialize(TileMap3& tiles, csg::Point3 const& tile_size, dm::Store& store);

   csg::Point3 GetTileSize() const;
   Region3BoxedPtr GetTile(csg::Point3 const& index, bool add_if_not_found = true);
   void ClearTile(csg::Point3 const& index);
   void OptimizeChangedTiles();

   void AddPoint(csg::Point3f const& point, int tag = 0);
   void AddCube(csg::Cube3f const& cube);
   void AddRegion(csg::Region3f const& region);

   csg::Region3f IntersectPoint(csg::Point3f const& point);
   csg::Region3f IntersectCube(csg::Cube3f const& cube);
   csg::Region3f IntersectRegion(csg::Region3f const& region);

   void SubtractPoint(csg::Point3f const& point);
   void SubtractCube(csg::Cube3f const& cube);
   void SubtractRegion(csg::Region3f const& region);

   friend std::ostream& operator<<(std::ostream& out, TiledRegion const& tiled_region);

private:
   TileMap3* _tiles;
   csg::Point3 _tile_size;
   dm::Store* _store;
   std::unordered_set<csg::Point3, csg::Point3::Hash> _unoptimized_tiles;
};

std::ostream& operator<<(std::ostream& out, TiledRegion const& tiled_region);

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_TILED_REGION_H
