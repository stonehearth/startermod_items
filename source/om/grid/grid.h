#ifndef _RADIANT_OM_TYPES_GRID_H
#define _RADIANT_OM_TYPES_GRID_H

#include "metrics.h"
#include "math3d.h"
#include "math3d_collision.h"
#include "om/all_object_types.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/array.h"
#include "tile_address.h"
#include "iterator.h"
#include "grid_tile.h"
#include <array>
#include <set>
#include <vector>

BEGIN_RADIANT_OM_NAMESPACE

class GridTile;

class Grid : public dm::Record {
   public:
      DEFINE_OM_OBJECT_TYPE(Grid, grid);
      friend GridTile;

      void InitializeRecordFields() override;

      void Clear();
      bool Intersect(const math3d::ray3& ray, math3d::point3& pt, math3d::point3& normal, float& t) const;

      void getVoxelColor(unsigned char voxel, math3d::color3& c) const;
      void SetPaletteEntry(int i, int r, int g, int b);
      void SetPalette(const math3d::color3* colors, int len);

      unsigned char getVoxel(int x, int y, int z);
      unsigned char getVoxel(const math3d::ipoint3 &p) { return getVoxel(p.x, p.y, p.z); }
      unsigned char getVoxel(const math3d::point3 &p) { return getVoxel((int)p.x, (int)p.y, (int)p.z); }

      unsigned char getVoxelResident(int x, int y, int z) const;
      unsigned char getVoxelResident(const math3d::ipoint3 &p) const;

      void setVoxel(int x, int y, int z, unsigned char value);
      void setVoxel(const math3d::ipoint3 &p, unsigned char value) { setVoxel(p.x, p.y, p.z, value); }
      void setVoxel(const math3d::ibounds3 &bounds, unsigned char value);
            
      bool isEmpty(const math3d::ipoint3 &p) const;
      bool isEmpty(const math3d::ibounds3 &bounds) const;
      bool isEmpty(int x, int y, int z) const;
            
      bool isPassable(const math3d::ipoint3 &p) const;
      bool isPassable(const math3d::ibounds3 &bounds) const;

      const math3d::ibounds3 &GetBounds() const;
      void GrowBounds(const math3d::ipoint3 &pt);
      void GrowBounds(const math3d::ibounds3 &bounds);

      bool isLoaded(const math3d::ipoint3 &p) const;

      math3d::point3 getCentroid() const;

      typedef std::function<void (const math3d::ipoint3 &, GridTile &)> TileGeneratorFn;
      void SetTileGenerator(TileGeneratorFn fn);

      std::vector<math3d::ipoint3> FindAdjacentVoxels(const math3d::ipoint3 &seed, int padding) const;
      bool GrowSelection(std::set<math3d::ipoint3> &selection, std::set<math3d::ipoint3> &fringe, uchar voxel) const;
      void CreateBorder(const std::set<math3d::ipoint3> &selection, std::set<math3d::ipoint3> &fringe) const;

      void AttachLoadedTile(std::shared_ptr<GridTile> tile);

      float GetOcculsionEstimate(const math3d::ibounds3 &area) const;

      void loadTiles(const math3d::ibounds3 &bounds);
      void merge(const math3d::ipoint3 &offset, const Grid *other);
         
      const TileMap &GetTiles() const;

      const math3d::ibounds3 &getBounds() const;
      void setBounds(const math3d::ibounds3 &bounds);

      const std::shared_ptr<GridTile> getTileConst(int x, int y, int z) const;
      const std::shared_ptr<GridTile> getTileConst(const TileAddress &addr) const;

      std::shared_ptr<GridTile> lookup(TileAddress addr) const;

      GridIterator iterator() const;
      BoundsIterator iterator(const math3d::ibounds3 &bounds) const;

      void forEachTile(std::function < void (std::shared_ptr<GridTile> t) > fn) const {
         for (const auto& entry : _tiles) {
            std::shared_ptr<GridTile> tile = entry.second;
            ASSERT(tile);
            fn(tile);
         };
      }

   protected:
      std::shared_ptr<GridTile> CreateTile(const TileAddress &addr);
      void ConstructTile(const TileAddress &addr, std::shared_ptr<GridTile> tile);

   private:
      NO_COPY_CONSTRUCTOR(Grid);

   protected:
      dm::Boxed<math3d::ibounds3>      _bounds;
      TileMap                          _tiles;
      dm::Array<math3d::color3, 16>    palette_;

      mutable TileAddress                 _last_search;
      mutable std::shared_ptr<GridTile>   _last_result;
};

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_TYPES_GRID_H

