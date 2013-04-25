#ifndef _RADIANT_OM_GRID_ITERATOR_H
#define _RADIANT_OM_GRID_ITERATOR_H

#include "math3d.h"
#include "bricklib.h"
#include "tile_address.h"
#include "om/om.h"
#include "dm/map.h"

#define BOX_SIZE     16

BEGIN_RADIANT_OM_NAMESPACE

class GridTile;
class TileAddress;
class Grid;

typedef dm::Map<TileAddress, std::shared_ptr<GridTile>, TileAddress::Hash >      TileMap;

class TileIterator;
class BrickIterator;
class FaceIterator;
class EdgeIterator;

class GridIterator {
   public:
      GridIterator(const Grid &grid);

      // void forEachTile(std::function<void (const TileIterator &)> const; // <-- add this one?
      void forEachTile(std::function<void (const TileIterator &)>) const;
      void forEachBrick(std::function<void (const BrickIterator &)>) const;
      void forEachFace(std::function<void (const FaceIterator &)>) const;
      void forEachEdge(std::function<void (const EdgeIterator &)>) const;
      void forEachVisibleBrick(std::function<void (const BrickIterator &)>) const;
      void forEachVisibleFace(std::function<void (const FaceIterator &)>) const;
      void forEachVisibleEdge(std::function<void (const EdgeIterator&)>) const;

   protected:
      const Grid        &_grid;
      const TileMap     &_tiles;
};

class BoundsIterator {
   public:
      BoundsIterator(const Grid &grid, math3d::ibounds3 bounds);

      void forEachVisibleFace(std::function<void (const FaceIterator &)>) const;
      void forEachVisibleBrick(std::function<void (const BrickIterator &)>) const;
      void forEachBrick(std::function<void (const BrickIterator &)>) const;
      void forEachTile(std::function<void (const TileIterator &)> fn) const;

   protected:
      const Grid        &_grid;
      const TileMap     &_tiles;
      math3d::ibounds3  _bounds;
};

class TileIterator {
   public:
      TileIterator(const Grid &grid, const GridTile &t);
      TileIterator(const Grid &grid, const GridTile &t, const math3d::ibounds3 &bounds);
      inline const Grid &grid() const { return _grid; }
      inline const GridTile &tile() const { return _tile; }

      void forEachBrick(std::function<void (const BrickIterator &)>) const;
      void forEachFace(std::function<void (const FaceIterator&)> fn) const;
      void forEachEdge(std::function<void (const EdgeIterator&)> fn) const;
      void forEachVisibleBrick(std::function<void (const BrickIterator &)>) const;
      void forEachVisibleMagicBrick(std::function<void (const BrickIterator &)>) const;
      void forEachVisibleFace(std::function<void (const FaceIterator&)> fn) const;
      void forEachVisibleEdge(std::function<void (const EdgeIterator&)> fn) const;

   protected:
      const Grid              &_grid;
      const GridTile              &_tile;
      math3d::ibounds3        _bounds;
};

class BrickIterator {
   public:
      BrickIterator(const TileIterator &t, const math3d::ipoint3 &offset) : _tile(t), _offset(offset) { }
      inline const Grid &grid() const { return _tile.grid(); }
      inline const GridTile &tile() const { return _tile.tile(); }
      const uchar brick() const;
      const math3d::ipoint3 &offset() const { return _offset; }

      bool faceIsObscured(BrickLib::FaceId face) const;

      void forEachFace(std::function<void (const FaceIterator&)> fn) const;
      void forEachEdge(std::function<void (const EdgeIterator&)> fn) const;
      void forEachVisibleFace(std::function<void (const FaceIterator&)> fn) const;
      void forEachVisibleEdge(std::function<void (const EdgeIterator&)> fn) const;               
               
   protected:
      const TileIterator      &_tile;
      const math3d::ipoint3   &_offset;
};

class FaceIterator {
   public:
      FaceIterator(const BrickIterator &bi) : _bi(bi) { }
      const uchar brick() const { return _bi.brick(); }
      const math3d::ipoint3 &offset() const { return _bi.offset(); }
      math3d::ipoint3 location() const;

   public:
      BrickLib::FaceInfo     face;

   protected:
      const BrickIterator     &_bi;
};

class EdgeIterator {
   public:
      EdgeIterator(const math3d::point3 &p0_, const math3d::point3 &p1_) : p0(p0_), p1(p1_) { }

   public:
      math3d::point3   p0, p1;

};

class BrickIteratorHelper {
   public:
      virtual void forEachVisibleFace(const BrickIterator &bi, std::function<void (const FaceIterator&)> fn) const = 0;
      virtual void forEachEdge(std::function<void (const EdgeIterator&)> fn) const = 0;
};

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_GRID_ITERATOR_H
