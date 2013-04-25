#include "pch.h"
#include "iterator.h"
#include "grid.h"
#include "grid_tile.h"

using namespace ::radiant;
using namespace ::radiant::om;


GridIterator::GridIterator(const Grid &grid) :
   _grid(grid),
   _tiles(_grid.GetTiles())
{
}
   
BoundsIterator::BoundsIterator(const Grid &grid, math3d::ibounds3 bounds) :
   _grid(grid),
   _tiles(_grid.GetTiles()),
   _bounds(bounds)
{
}

void BoundsIterator::forEachVisibleFace(std::function<void (const FaceIterator &)> fn) const
{
   TileAddress::forEachAddress(_bounds, [&](const TileAddress &addr) {
      auto i = _tiles.find(addr);
      if (i != _tiles.end()) {
         math3d::ibounds3 bounds;
         bounds._min = addr.brickOffset(_bounds._min);
         bounds._max = addr.brickOffset(_bounds._max);
         for (int j = 0; j < 3; j++) {            
            bounds._min[j] = std::max(0, std::min(bounds._min[j], BOX_SIZE));
            bounds._max[j] = std::max(0, std::min(bounds._max[j], BOX_SIZE));
         }
         i->second->iterator(bounds).forEachVisibleFace(fn);
      }
   });   
}

void BoundsIterator::forEachVisibleBrick(std::function<void (const BrickIterator &)> fn) const
{
   forEachTile([&](const TileIterator &ti) {
      ti.forEachVisibleBrick(fn);
   });
}

void BoundsIterator::forEachBrick(std::function<void (const BrickIterator &)> fn) const
{
   forEachTile([&](const TileIterator &ti) {
      ti.forEachBrick(fn);
   });
}

void BoundsIterator::forEachTile(std::function<void (const TileIterator &)> fn) const
{
   TileAddress::forEachAddress(_bounds, [&](const TileAddress &addr) {
      auto i = _tiles.find(addr);
      if (i != _tiles.end()) {
         math3d::ibounds3 bounds;
         bounds._min = addr.brickOffset(_bounds._min);
         bounds._max = addr.brickOffset(_bounds._max);
         for (int j = 0; j < 3; j++) {            
            bounds._min[j] = std::max(0, std::min(bounds._min[j], BOX_SIZE));
            bounds._max[j] = std::max(0, std::min(bounds._max[j], BOX_SIZE));
         }
         std::shared_ptr<GridTile> tile = i->second;
         fn(TileIterator(_grid, *tile, bounds));
      }
   });
}


void GridIterator::forEachBrick(std::function<void (const BrickIterator &)> fn) const
{
   for (const auto& entry : _tiles) {
      entry.second->iterator().forEachBrick(fn);
   }
}


void GridIterator::forEachVisibleBrick(std::function<void (const BrickIterator &)> fn) const
{
   for (const auto& entry : _tiles) {
      entry.second->iterator().forEachVisibleBrick(fn);
   }
}

// make sure we offset by the origin!
void GridIterator::forEachVisibleFace(std::function<void (const FaceIterator&)> fn) const
{
   for (const auto& entry : _tiles) {
      entry.second->iterator().forEachVisibleFace(fn);
   }
}

TileIterator::TileIterator(const Grid &grid, const GridTile &t) :
   _grid(grid),
   _tile(t),
   _bounds(math3d::ipoint3(0, 0, 0), math3d::ipoint3(BOX_SIZE, BOX_SIZE, BOX_SIZE))
{
}

TileIterator::TileIterator(const Grid &grid, const GridTile &t, const math3d::ibounds3 &bounds) :
   _grid(grid),
   _tile(t),
   _bounds(bounds)
{
   ASSERT(_bounds._min.x >= 0 && _bounds._min.y >= 0 && _bounds._min.z >= 0);
   ASSERT(_bounds._max.x <= BOX_SIZE && _bounds._max.y <= BOX_SIZE && _bounds._max.z <= BOX_SIZE);
}


// Iterate over every brick in this tile.
void TileIterator::forEachBrick(std::function<void (const BrickIterator&)> fn) const
{
   math3d::ipoint3 offset;

   for (offset.z = _bounds._min.z; offset.z < _bounds._max.z; offset.z++) {
      for (offset.y = _bounds._min.y; offset.y < _bounds._max.y; offset.y++) {
         for (offset.x = _bounds._min.x; offset.x < _bounds._max.x; offset.x++) {
            if (!_tile.isEmpty(offset)) {
               fn(BrickIterator(*this, offset));
            }
         }
      }
   }
}

// Iterate over every visbile brick in this tile.
void TileIterator::forEachVisibleBrick(std::function<void (const BrickIterator&)> fn) const
{
   forEachBrick([&](const BrickIterator &b) {
      for (unsigned int i = 0; i < 6; i++) {
         if (!b.faceIsObscured((BrickLib::FaceId)i)) {
            fn(b);
            return;
         }
      }
   });
}

void TileIterator::forEachVisibleMagicBrick(std::function<void (const BrickIterator&)> fn) const
{
   forEachBrick([&](const BrickIterator &b) {
      if (BrickLib::isMagicBrick(b.brick())) {
         for (unsigned int i = 0; i < 6; i++) {
            if (!b.faceIsObscured((BrickLib::FaceId)i)) {
               fn(b);
               return;
            }
         }
      }
   });
}


void TileIterator::forEachVisibleFace(std::function<void (const FaceIterator&)> fn) const
{
   // we use forEachBrick here instead of forEachVisibleBrick to avoid duplicating
   // the bi.faceIsObsured check.  In the case where only 1 face is visible the
   // other 5 checks against obscured faces could be potentially duplicated.

   forEachBrick([&](const BrickIterator &bi) {
      FaceIterator fi(bi);

      if (!BrickLib::isMagicBrick(bi.brick())) {
         for (int i = 0; i < 6; i++) {
            BrickLib::FaceId faceId = (BrickLib::FaceId)i;
            if (!bi.faceIsObscured(faceId)) {
               BrickLib::addQuad(fi.face, faceId);
            }
         }
         if (fi.face.indices.size() > 0) {
            fn(fi);
         }
      }
   });
}

const radiant::uchar BrickIterator::brick() const
{
   return tile().getVoxel(_offset);
}

bool BrickIterator::faceIsObscured(BrickLib::FaceId face) const
{
   math3d::ipoint3 neighbor = BrickLib::getNeighbor(tile().getLocation() + _offset, face);
   BrickLib::FaceId oppositeSide = BrickLib::getFacing(face);

   if (!grid().GetBounds().contains(neighbor)) {
      return false;
   }
   if (!grid().isLoaded(neighbor)) {
      return false;
   }
   radiant::uchar voxel = grid().getVoxelResident(neighbor);
   return BrickLib::isOcculder(voxel);
}

// Iterate over every face in this brick
void BrickIterator::forEachFace(std::function<void (const FaceIterator&)> fn) const
{
   ASSERT(false);
}

math3d::ipoint3 FaceIterator::location() const
{
   return _bi.offset() + _bi.tile().getLocation();
}

// make sure we offset by the origin!
void BrickIterator::forEachVisibleEdge(std::function<void (const EdgeIterator&)> fn) const
{
   ASSERT(brick() != 0);

   // xxx: store these as edge_0 - edge_11
   fn(EdgeIterator(BrickLib::point_0, BrickLib::point_1));
   fn(EdgeIterator(BrickLib::point_1, BrickLib::point_2));
   fn(EdgeIterator(BrickLib::point_2, BrickLib::point_3));
   fn(EdgeIterator(BrickLib::point_3, BrickLib::point_0));

   fn(EdgeIterator(BrickLib::point_4, BrickLib::point_5));
   fn(EdgeIterator(BrickLib::point_5, BrickLib::point_6));
   fn(EdgeIterator(BrickLib::point_6, BrickLib::point_7));
   fn(EdgeIterator(BrickLib::point_7, BrickLib::point_4));

   fn(EdgeIterator(BrickLib::point_0, BrickLib::point_4));
   fn(EdgeIterator(BrickLib::point_1, BrickLib::point_5));
   fn(EdgeIterator(BrickLib::point_2, BrickLib::point_6));
   fn(EdgeIterator(BrickLib::point_3, BrickLib::point_7));
}

