#include "pch.h"
#include "grid_tile.h"
#include "grid.h"
#include "snappy.h"
#include "metrics.h"

using namespace ::radiant;
using namespace ::radiant::om;

const int GridTile::size = 16;
   
GridTile::~GridTile()
{
   LOG(WARNING) << "CONSTRUCTING NEW GRID TILE";
}

void GridTile::InitializeRecordFields() 
{
   AddRecordField("grid", _grid);
   AddRecordField("location", _location);
   AddRecordField("cellCount", cellCount_);
}

const math3d::ipoint3 GridTile::getLocation() const
{
   return _location;
}

radiant::uchar GridTile::getVoxel(int x, int y, int z) const
{
   ASSERT(x >= 0 && y >= 0 && z >= 0 &&
          x < BOX_SIZE && y < BOX_SIZE && z < BOX_SIZE);
   return _cells[y][x][z];
}

void GridTile::SetGrid(std::shared_ptr<Grid> grid)
{
   _grid = grid;
}

GridPtr GridTile::GetGrid()
{
   return (*_grid).lock();
}


void GridTile::SetLocation(const math3d::ipoint3 &pt)
{
   _location = pt;
}


bool GridTile::isEmpty(int x, int y, int z) const
{
   return getVoxel(x, y, z) == 0;
}

void GridTile::clear()
{
   zero();
   MarkChanged();
}

void GridTile::zero()
{
   cellCount_ = 0;
   memset(_cells, 0, sizeof(_cells));
}

void GridTile::setVoxel(int x, int y, int z, radiant::uchar value)
{
   ASSERT(x >= 0 && y >= 0 && z >= 0);
   ASSERT(x < GridTile::size && y < GridTile::size && z < GridTile::size);
   unsigned char current = _cells[y][x][z];
   if (current && !value) {
      cellCount_ = cellCount_ - 1;
   } else if (!current && value) {
      cellCount_ = cellCount_ + 1;
   }
   _cells[y][x][z] = value;
   MarkChanged();
}

void GridTile::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   dm::Record::SaveValue(store, msg);

   Protocol::GridTile* tile = msg->MutableExtension(Protocol::GridTile::tile);

   std::string c;
   c = compress((radiant::uchar *)_cells, sizeof(_cells));
   tile->set_cells(c.data(), c.size());

   LOG(WARNING) << "Saving grid tile " << store.GetStoreId() << ", " << GetObjectId();
}

void GridTile::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   dm::Record::LoadValue(store, msg);

   const Protocol::GridTile& tile = msg.GetExtension(Protocol::GridTile::tile);

   decompress(tile.cells(), (radiant::uchar *)_cells, sizeof(_cells));

   LOG(WARNING) << "Loading grid tile " << store.GetStoreId() << ", " << GetObjectId();
}

void GridTile::CloneObject(dm::Object* c, dm::CloneMapping& mapping) const
{
   dm::Record::CloneObject(c, mapping);
   GridTile& copy = static_cast<GridTile&>(*c);
   memcpy(copy._cells, _cells, sizeof _cells);
}

bool GridTile::decompress(std::string compressed, radiant::uchar *dst, int dstLen)
{
   PROFILE_BLOCK();

   size_t len;
   bool success = false;
   if (snappy::GetUncompressedLength(compressed.data(), compressed.size(), &len)) {
      ASSERT(len == dstLen);
      if (len == dstLen) {
         snappy::RawUncompress(compressed.data(), compressed.size(), (char *)dst);
         success = true;
      }
   }
   return success;
}

std::string GridTile::compress(radiant::uchar *src, int len) const
{
   PROFILE_BLOCK();
   std::string compressed;

   size_t c = snappy::Compress((const char *)src, len, (std::string *)&compressed);
   float percent = (c * 100.0f) / len;
   DLOG(INFO) << "compressed cells down to " << compressed.size() << " bytes (" << percent << " %)";

   return std::move(compressed); // xxx - std::move necessary here?
}

TileIterator GridTile::iterator() const
{
   return TileIterator(*(*_grid).lock(), *this);
}

TileIterator GridTile::iterator(const math3d::ibounds3 &bounds) const
{
   return TileIterator(*(*_grid).lock(), *this, bounds);
}

bool GridTile::Intersect(const math3d::ray3& ray, math3d::point3& pt, math3d::point3& normal, float &best /* in-out */)
{
   float t;
   bool result = false;
   math3d::point3 o(_location);

   iterator().forEachVisibleFace([&](const FaceIterator& fi) {
      const BrickLib::FaceInfo& info = fi.face;
      for (unsigned int i = 0; i < info.indices.size(); i += 3) {
         math3d::point3 p0 = o + info.vertices[info.indices[i]].position;
         math3d::point3 p1 = o + info.vertices[info.indices[i+1]].position;
         math3d::point3 p2 = o + info.vertices[info.indices[i+2]].position;
         if (math3d::triangle_intersect(t, p0, p1, p2, ray) && t < best) {
            best = t;
            pt = ray.origin + ray.direction * t;
            normal = info.vertices[info.indices[i]].normal;
            result = true;
         }
      }
   });
   return result;
}
