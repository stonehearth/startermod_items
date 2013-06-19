#include "pch.h"
#include <sstream>
#include "grid.h"
#include "grid_tile.h"
#include "snappy.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Grid::InitializeRecordFields() 
{
   AddRecordField("bounds", _bounds);
   AddRecordField("tiles", _tiles);
   AddRecordField("palette", palette_);

   _last_search = TileAddress(INT_MAX, INT_MAX, INT_MAX);
   _bounds.Modify().set_zero();
}

#if 0
NotifyChangeFnId Grid::SetNotifyTileChanged(std::function<void(GridTile *)> fn) const
{
   auto cb = [fn](std::shared_ptr<Object> obj) {
      ASSERT(obj->GetObjectType() == GridTile::Type);
      fn(CastObject<GridTile>(obj).get());
   };

   return _manager->SetNotifyTileChanged(_id, cb);
}

void Grid::CancelNotifyTileChanged(NotifyChangeFnId id) const
{
   _manager->CancelNotifyChanges(id);
}
#endif

const math3d::ibounds3 &Grid::getBounds() const
{
   return _bounds;
}

void Grid::setBounds(const math3d::ibounds3 &bounds)
{
   _bounds = bounds;
}

math3d::point3 Grid::getCentroid() const
{
   return (*_bounds).centroid();
}

const TileMap &Grid::GetTiles() const
{
   return _tiles;
}

radiant::uchar Grid::getVoxel(int x, int y, int z)
{
   if (!(*_bounds).contains(math3d::ipoint3(x, y, z))) {
      return 0;
   }

   auto addr = TileAddress(x, y, z);
   return CreateTile(addr)->getVoxel(addr.brick_address(x, y, z));
}

/*
 * Caller needs to get this call by isLoaded(pt)
 */
radiant::uchar Grid::getVoxelResident(int x, int y, int z) const
{
   return getVoxelResident(math3d::ipoint3(x, y, z));
}

radiant::uchar Grid::getVoxelResident(const math3d::ipoint3 &p) const
{
   auto addr = TileAddress(p);
   std::shared_ptr<GridTile> tile = lookup(addr);

   // xxx: return a solid color
   return tile ? tile->getVoxel(addr.brick_address(p)) : 0;
}

bool Grid::isEmpty(const math3d::ipoint3 &p) const
{
   if (!(*_bounds).contains(p)) {
      return true;
   }
   auto addr = TileAddress(p);
   std::shared_ptr<GridTile> tile = lookup(addr);
   return tile ? tile->isEmpty(addr.brick_address(p)) : true;
}

bool Grid::isEmpty(const math3d::ibounds3 &b) const
{
   return b.filter_each_point([&](const math3d::ipoint3 &pt) { return isEmpty(pt); });
}

bool Grid::isEmpty(int x, int y, int z) const
{
   return isEmpty(math3d::ipoint3(x, y, z));
}

bool Grid::isPassable(const math3d::ipoint3 &p) const
{
   if (!(*_bounds).contains(math3d::ipoint3(p))) {
      return true;
   }
   return BrickLib::isPassable(getVoxelResident(p));
}

bool Grid::isPassable(const math3d::ibounds3 &b) const
{
   return b.filter_each_point([&](const math3d::ipoint3 &pt) { return isPassable(pt); });
}

bool Grid::isLoaded(const math3d::ipoint3 &p) const
{
   return (*_bounds).contains(p) && lookup(TileAddress(p)) != NULL;
}

void Grid::setVoxel(int x, int y, int z, radiant::uchar value)
{
   ASSERT((*_bounds).contains(math3d::ipoint3(x, y, z)));

   auto addr = TileAddress(x, y, z);
   auto tile = CreateTile(addr);

   tile->setVoxel(addr.brick_address(x, y, z), value);
}

void Grid::setVoxel(const math3d::ibounds3 &bounds, unsigned char value)
{
   bounds.for_each_point([&](const math3d::ipoint3 &pt) {
      setVoxel(pt, value);
   });
}

const math3d::ibounds3 &Grid::GetBounds() const
{
   return (*_bounds);
}

void Grid::GrowBounds(const math3d::ipoint3 &pt)
{
   GrowBounds(math3d::ibounds3(pt, pt + math3d::ipoint3(1, 1, 1)));
}

void Grid::GrowBounds(const math3d::ibounds3 &bounds)
{
   _bounds = _bounds.Modify().merge(bounds); // xxx: this is a horrible interface for merge
}

void Grid::loadTiles(const math3d::ibounds3 &bounds)
{
   TileAddress::forEachAddress(bounds, [&](const TileAddress &addr) {
      if ((*_bounds).contains(math3d::ipoint3(addr))) {
         if (!lookup(addr)) {
            CreateTile(addr);
         }
      }
   });
}

void Grid::merge(const math3d::ipoint3 &origin, const Grid *other)
{
   other->iterator().forEachBrick([&](const BrickIterator &bi) {
      math3d::ipoint3 &pt = origin + bi.offset();
      if (isEmpty(pt)) {
         setVoxel(pt, bi.brick());
      }
   });
}

const std::shared_ptr<GridTile> Grid::getTileConst(const TileAddress &addr) const
{
   return lookup(addr);
}

std::shared_ptr<GridTile> Grid::CreateTile(const TileAddress &addr)
{
   std::shared_ptr<GridTile> tile;

   tile = lookup(addr);
   if (!tile) {
      tile = GetStore().AllocObject<GridTile>();
      ConstructTile(addr, tile);
   }
   return tile;
}

void Grid::AttachLoadedTile(std::shared_ptr<GridTile> tile)
{
   TileAddress addr(tile->getLocation());
   auto resident = lookup(addr);
   if (resident != tile) {
      ASSERT(!resident);
      ConstructTile(addr, tile);
   }
}

void Grid::ConstructTile(const TileAddress &addr, std::shared_ptr<GridTile> tile)
{
   ASSERT(_tiles.find(addr) == _tiles.end());

   tile->SetLocation(math3d::ipoint3(addr));
   tile->SetGrid(GetStore().FetchObject<Grid>(GetObjectId()));
   tile->zero();

   _tiles[addr] = tile;   

   math3d::ibounds3 &bounds = _bounds.Modify();
   if (bounds._min == bounds._max) {
      bounds._min = addr.origin();
      bounds._max = addr.origin();
   } else {
      bounds.merge(addr);
   }
   bounds.merge(addr.origin() + math3d::ipoint3(16, 16, 16));

   if (addr == _last_search) {
      _last_result = tile;
   }
}

GridIterator Grid::iterator() const
{
   return GridIterator(*this);
}

BoundsIterator Grid::iterator(const math3d::ibounds3 &bounds) const
{
   return BoundsIterator(*this, bounds);
}

std::shared_ptr<GridTile> Grid::lookup(TileAddress addr) const
{
#if 0
   if (!(*_bounds).contains(math3d::ipoint3(addr))) {
      LOG(WARNING) << "OOBBBB!";
   }
#endif

   int id = GetObjectId();
   if (addr != _last_search) {
      auto i = _tiles.find(addr);
      _last_search = addr;
      if (i == _tiles.end()) {
         _last_result = nullptr;
      } else {
         _last_result = i->second;
         ASSERT(_last_result);
         ASSERT(i->first == addr);
         ASSERT(_last_result->getLocation() == math3d::ipoint3(addr));
      }
   }

   ASSERT(!_last_result || _last_result->getLocation() == math3d::ipoint3(addr));

   return _last_result;
}

float Grid::GetOcculsionEstimate(const math3d::ibounds3 &bounds) const
{
   int total = 0;
   iterator(bounds).forEachBrick([&](const BrickIterator &bi) {
      total += 1;
   });
   return ((float)total) / bounds.area();
}

vector<math3d::ipoint3> Grid::FindAdjacentVoxels(const math3d::ipoint3 &seed, int padding) const
{
   bool finished = false;
   set<math3d::ipoint3> adjacent, fill, fringe;
   radiant::uchar voxel = getVoxelResident(seed);

   fill.insert(seed);
   while (!finished) {
      fringe.clear();
      finished = GrowSelection(fill, fringe, voxel);
      fill.insert(fringe.begin(), fringe.end());
   }
   for (int i = 0; i < padding; i++) {
      fringe.clear();
      CreateBorder(fill, fringe);
      fill.insert(fringe.begin(), fringe.end());
   }

   // Now build the margin set by growing 1 more time.
   fringe.clear();
   CreateBorder(fill, fringe);

   return vector<math3d::ipoint3>(fringe.begin(), fringe.end());
}

bool Grid::GrowSelection(set<math3d::ipoint3> &selection, set<math3d::ipoint3> &fringe, radiant::uchar voxel) const
{
   static const math3d::ipoint3 offsets[] = {
      math3d::ipoint3( 1, 0, 0),
      math3d::ipoint3( 0, 0, 1),
      math3d::ipoint3(-1, 0, 0),
      math3d::ipoint3( 0, 0, -1),
   };

   for_each(begin(selection), end(selection), [&](const math3d::ipoint3 &pt) {
      for (int i = 0; i < ARRAYSIZE(offsets); i++) {
         math3d::ipoint3 offset = pt +  offsets[i];
         if ((voxel == 0 || getVoxelResident(offset) == voxel) && !radiant::stdutil::contains(selection, offset)) {
            fringe.insert(offset);
         }
      }
   });
   return fringe.size() == 0;
}

void Grid::CreateBorder(const set<math3d::ipoint3> &selection, set<math3d::ipoint3> &fringe) const
{
   static const math3d::ipoint3 offsets[] = {
      math3d::ipoint3(  1,  0,  0),
      math3d::ipoint3(  1,  0,  1),
      math3d::ipoint3(  0,  0,  1),
      math3d::ipoint3( -1,  0,  1),
      math3d::ipoint3( -1,  0,  0),
      math3d::ipoint3( -1,  0, -1),
      math3d::ipoint3(  0,  0, -1),
      math3d::ipoint3(  1,  0, -1),
   };

   for_each(begin(selection), end(selection), [&](const math3d::ipoint3 &pt) {
      for (int i = 0; i < ARRAYSIZE(offsets); i++) {
         math3d::ipoint3 offset = pt +  offsets[i];
         if (!radiant::stdutil::contains(selection, offset)) {
            fringe.insert(offset);
         }
      }
   });
}

bool Grid::Intersect(const math3d::ray3& ray, math3d::point3& pt, math3d::point3& normal, float &best) const
{
   float t;
   bool result = false;
   math3d::aabb box(GetBounds());

   if (box.Intersects(t, ray) && t < best) {
      // xxx: this is absurdly slow...
      for (const auto& entry : _tiles) {
         box._min = entry.first;
         box._max = box._min + math3d::point3(16, 16, 16);
         if (box.Intersects(t, ray) && t < best) {
            std::shared_ptr<GridTile> tile = entry.second;
            if (tile->Intersect(ray, pt, normal, best)) {
               result = true;
            }
         }
      }
   }
   return result;
}

void Grid::Clear()
{
   _tiles.Clear();
   _bounds.Modify().set_zero();
   _last_search = TileAddress(INT_MAX, INT_MAX, INT_MAX);
   _last_result = nullptr;
}

void Grid::getVoxelColor(radiant::uchar voxel, math3d::color3& c) const
{
   c = palette_[voxel];
}

void Grid::SetPaletteEntry(int i, int r, int g, int b)
{
   palette_[i] = math3d::color3(r, g, b);
}


void Grid::SetPalette(const math3d::color3* colors, int len)
{
   for (int i = 0; i < len; i++) {
      palette_[i] = colors[i];
   }
}
