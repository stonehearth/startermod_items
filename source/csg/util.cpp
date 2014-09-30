#include "pch.h"
#include "util.h"

using namespace ::radiant;
using namespace ::radiant::csg;

int csg::ToInt(int i)
{
   return i;
}

int csg::ToInt(double s)
{
   if (s > 0) {
      return static_cast<int>(s + DBL_EPSILON);
   }
   return static_cast<int>(s - DBL_EPSILON);
}

int csg::ToClosestInt(int i)
{
   return i;
}

int csg::ToClosestInt(double s) {
   if (s > 0) {
      return static_cast<int>(s + 0.5f + DBL_EPSILON);
   }
   return static_cast<int>(s - 0.5f - DBL_EPSILON);
}

int csg::GetChunkAddressSlow(int value, int width)
{
   return GetChunkIndexSlow(value, width) * width;
}

int csg::GetChunkIndexSlow(int value, int width)
{
   return (int) std::floor((double)value / width);
}

void csg::GetChunkIndexSlow(int value, int chunk_width, int& index, int& offset)
{
   index = csg::GetChunkIndexSlow(value, chunk_width);
   offset = value - (index * chunk_width);
   ASSERT(offset >= 0 && offset < chunk_width);
}

Point3 csg::GetChunkIndexSlow(Point3 const& value, int chunk_width)
{
   return Point3(GetChunkIndexSlow(value.x, chunk_width),
                 GetChunkIndexSlow(value.y, chunk_width),
                 GetChunkIndexSlow(value.z, chunk_width));
}

Point3 csg::GetChunkIndexSlow(Point3 const& value, Point3 const& chunk)
{
   return Point3(GetChunkIndexSlow(value.x, chunk.x),
                 GetChunkIndexSlow(value.y, chunk.y),
                 GetChunkIndexSlow(value.z, chunk.z));
}

void csg::GetChunkIndexSlow(Point3 const& value, int chunk_width, Point3& index, Point3& offset)
{
   GetChunkIndexSlow(value.x, chunk_width, index.x, offset.x);
   GetChunkIndexSlow(value.y, chunk_width, index.y, offset.y);
   GetChunkIndexSlow(value.z, chunk_width, index.z, offset.z);
}

void csg::GetChunkIndexSlow(Point3 const& value, Point3 const& chunk, Point3& index, Point3& offset)
{
   GetChunkIndexSlow(value.x, chunk.x, index.x, offset.x);
   GetChunkIndexSlow(value.y, chunk.y, index.y, offset.y);
   GetChunkIndexSlow(value.z, chunk.z, index.z, offset.z);
}

Cube3 csg::GetChunkIndexSlow(Cube3 const& value, int chunk_width)
{
   Point3 ceil(chunk_width - 1, chunk_width - 1, chunk_width - 1);
   Cube3 index = Cube3(GetChunkIndexSlow(value.min, chunk_width),
                       GetChunkIndexSlow(value.max + ceil, chunk_width),
                       value.GetTag());
   return index;
}

Cube3 csg::GetChunkIndexSlow(Cube3 const& value, Point3 const& chunk)
{
   Point3 ceil(chunk.x - 1, chunk.y - 1, chunk.z - 1);
   Cube3 index = Cube3(GetChunkIndexSlow(value.min, chunk),
                       GetChunkIndexSlow(value.max + ceil, chunk),
                       value.GetTag());
   return index;
}

bool csg::PartitionCubeIntoChunksSlow(Cube3 const& cube, int width, std::function<bool (Point3 const& index, Cube3 const& cube)> const& cb)
{
   return PartitionCubeIntoChunksSlow(cube, Point3(width, width, width), cb);
}

bool csg::PartitionCubeIntoChunksSlow(Cube3 const& cube, Point3 const& chunk, std::function<bool (Point3 const& index, Cube3 const& cube)> const& cb)
{
   Point3 const& cmin = cube.GetMin();
   Point3 const& cmax = cube.GetMax();
   Cube3 chunks = GetChunkIndexSlow(cube, chunk);

   for (Point3 const& cursor : chunks) {
      Cube3 c;
      for (int i = 0; i < 3; i++) {
         c.min[i] = std::max(cmin[i], cursor[i] * chunk[i]);
         c.max[i] = std::min(cmax[i], (cursor[i] + 1) * chunk[i]);
      }
      c.SetTag(cube.GetTag());

      // sometimes this generates cubes of 0 dimensions. not sure why yet...
      if (c.GetArea() > 0) {
         bool stop = cb(cursor, c);
         if (stop) {
            return true;
         }
      }
   }
   return false;
}

bool csg::PartitionRegionIntoChunksSlow(Region3 const& region, int width, std::function<bool(Point3 const& index, Region3 const& r)> cb)
{
   return PartitionRegionIntoChunksSlow(region, Point3(width, width, width), cb);
}

bool csg::PartitionRegionIntoChunksSlow(Region3 const& region, Point3 const& chunk, std::function<bool(Point3 const& index, Region3 const& r)> cb)
{
   std::unordered_map<Point3, Region3, Point3::Hash> regions;
   for (Cube3 const& cube : region) {
      PartitionCubeIntoChunksSlow(cube, chunk, [&regions](Point3 const& index, Cube3 const& cube) mutable {
         regions[index].AddUnique(cube);
         return false;
      });
   }
   bool stopped = false;
   for (auto const& entry : regions) {
      stopped = cb(entry.first, entry.second);
      if (stopped) {
         break;
      }
   }
   return stopped;
}


// Specializations first!
template<> inline int csg::GetChunkIndex<16>(int value)
{
   return value >> 4;
}

template<> inline void csg::GetChunkIndex<16>(int value, int& index, int& offset)
{
   index = value >> 4;
   offset = value & 15;
}

template<> inline int csg::GetChunkIndex<32>(int value)
{
   return value >> 5;
}

template<> inline void csg::GetChunkIndex<32>(int value, int& index, int& offset)
{
   index = value >> 5;
   offset = value & 31;
}

template<> inline int csg::GetChunkIndex<64>(int value)
{
   return value >> 6;
}

template<> inline void csg::GetChunkIndex<64>(int value, int& index, int& offset)
{
   index = value >> 6;
   offset = value & 63;
}

template<> inline int csg::GetChunkIndex<128>(int value)
{
   return value >> 7;
}

template<> inline void csg::GetChunkIndex<128>(int value, int& index, int& offset)
{
   index = value >> 7;
   offset = value & 127;
}

// Now the rest...
template <int S> 
inline int csg::GetChunkAddress(int value)
{
   return GetChunkIndex<S>(value) * S;
}

template <int S> 
inline int csg::GetChunkIndex(int value)
{
   return (int) std::floor((double)value / S);
}

template <int S> 
inline void csg::GetChunkIndex(int value, int& index, int& offset)
{
   index = csg::GetChunkIndex<S>(value);
   offset = value - (index * S);
   ASSERT(offset >= 0 && offset < S);
}

template <int S> 
inline Point3 csg::GetChunkIndex(Point3 const& value)
{
   return Point3(GetChunkIndex<S>(value.x),
                 GetChunkIndex<S>(value.y),
                 GetChunkIndex<S>(value.z));
}

template <int S> 
inline void csg::GetChunkIndex(Point3 const& value, Point3& index, Point3& offset)
{
   GetChunkIndex<S>(value.x, index.x, offset.x);
   GetChunkIndex<S>(value.y, index.y, offset.y);
   GetChunkIndex<S>(value.z, index.z, offset.z);
}

template <int S> 
inline Cube3 csg::GetChunkIndex(Cube3 const& value)
{
   Point3 ceil(S - 1, S - 1, S - 1);
   Cube3 index = Cube3(GetChunkIndex<S>(value.min),
                       GetChunkIndex<S>(value.max + ceil),
                       value.GetTag());
   return index;
}

template <int S>
bool csg::PartitionCubeIntoChunks(Cube3 const& cube, std::function<bool (Point3 const& index, Cube3 const& cube)> cb)
{
   Point3 const& cmin = cube.GetMin();
   Point3 const& cmax = cube.GetMax();
   Cube3 chunks = GetChunkIndex<S>(cube);

   for (Point3 const& cursor : chunks) {
      Cube3 c;
      for (int i = 0; i < 3; i++) {
         c.min[i] = std::max(cmin[i] - cursor[i] * S, 0);
         c.max[i] = std::min(cmax[i] - cursor[i] * S, S);
      }
      c.SetTag(cube.GetTag());

      // sometimes this generates cubes of 0 dimensions. not sure why yet...
      if (c.GetArea() > 0) {
         bool stop = cb(cursor, c);
         if (stop) {
            return true;
         }
      }
   }
   return false;
}

Region3 csg::Reface(Region3 const& rgn, Point3 const& forward)
{
   Region3 result;   
   int cos_theta, sin_theta, tx = 0, tz = 0;

   if (forward == Point3(0, 0, -1)) { // 0...
      return rgn;
   } else if (forward == Point3(1, 0, 0)) { // 90...
      cos_theta = 0;
      sin_theta = 1;
      tx = 1;
   } else if (forward == Point3(0, 0, 1)) { // 180...
      cos_theta = -1; 
      sin_theta = 0;
      tx = 1;
      tz = 1;
   } else if (forward == Point3(-1, 0, 0)) { // 270...
      cos_theta = 0; 
      sin_theta = -1;
      tz = 1;
   } else {
      throw std::invalid_argument(BUILD_STRING("vector is not a valid normal refacing region: " << forward));
   }

   for (const auto& cube : rgn) {
      Point3 const& min = cube.GetMin();
      Point3 const& max = cube.GetMax();
      result.AddUnique(Cube3::Construct(Point3(min.x * cos_theta - min.z * sin_theta + tx,
                                               min.y,
                                               min.x * sin_theta + min.z * cos_theta + tz),
                                        Point3(max.x * cos_theta - max.z * sin_theta + tx,
                                               max.y,
                                               max.x * sin_theta + max.z * cos_theta + tz),
                                        cube.GetTag()));
   }
   return result;
}


template <typename Region>
Region csg::GetAdjacent(Region const& r, bool allow_diagonals)
{
   Region adjacent;

   for (const Region::Cube& c : r) {
      Region::Point p0 = c.GetMin();
      Region::Point p1 = c.GetMax();
      if (allow_diagonals) {
         adjacent.Add(Region::Cube(p0 + Region::Point(-1, 0, -1), p1 + Region::Point(1, 0, 1)));
      } else {
         Region::Point delta = p1 - p0;
         Region::ScalarType x = delta.x, y = delta.y, z = delta.z;

         adjacent.Add(Region::Cube(p0 - Region::Point(0, 0, 1), p0 + Region::Point(x, y, 0)));     // top
         adjacent.Add(Region::Cube(p0 - Region::Point(1, 0, 0), p0 + Region::Point(0, y, z)));     // left
         adjacent.Add(Region::Cube(p0 + Region::Point(0, 0, z), p0 + Region::Point(x, y, z + 1))); // bottom
         adjacent.Add(Region::Cube(p0 + Region::Point(x, 0, 0), p0 + Region::Point(x + 1, y, z))); // right
      }
   }

   adjacent.OptimizeByMerge();
   return adjacent;
}

void csg::HeightmapToRegion2f(HeightMap<double> const& h, Region2f& r)
{
   HeightMap<int> n(h);
   int size = n.get_size();
   int scale = n.get_scale();
   auto samples = n.get_samples();

   int search_offset = 0;
   int search_len = size * size;
   
   while (search_offset < search_len) {
      int search_value = samples[search_offset];
      if (search_value == INT_MAX) {
         search_offset++;
         continue;
      }
      int y0 = search_offset / size;
      int x0 = search_offset % size;
      int x1 = x0 + 1;
      int y1 = y0 + 1;
      while (x1 < size && samples[y0 * size + x1] == search_value) {
         x1++;
      }
      while (y1 < size) {
         int i;
         for (i = x0; i < x1; i++) {
            if (samples[y1 * size + i] != search_value) {
               break;
            }
         }
         if (i != x1) {
            break;
         }
         y1++;
      }

      // yay! add the rect, rounding to the nearest whole number
      r.AddUnique(Rect2f(csg::ToFloat(Point2(x0 * scale, y0 * scale)),
                         csg::ToFloat(Point2(x1 * scale, y1 * scale)),
                         search_value));

      // clear the rect
      for (int y = y0; y < y1; y++) {
         for (int x = x0; x < x1; x++) {
            samples[y * size + x] = INT_MAX;
         }
      }
      search_offset += (x1 - x0);
   }
}

bool ::csg::Region3Intersects(const Region3& rgn, const csg::Ray3& ray, double& distance)
{
   double best = FLT_MAX;
   for (const auto &r : rgn) { 
      double candidate;
      if (Cube3Intersects(r, ray, candidate) && candidate < best) {
         best = candidate;
      }
   }
   if (best != FLT_MAX) {
      distance = best;
      return true;
   }
   return false;
}

template <typename S> Point<S, 2> csg::ProjectOntoXY(Point<S, 3> const& pt)
{
   return Point<S, 2>(pt.x, pt.y);
}

template <typename S> Point<S, 2> csg::ProjectOntoXZ(Point<S, 3> const& pt)
{
   return Point<S, 2>(pt.x, pt.z);
}

template <typename S> Point<S, 2> csg::ProjectOntoYZ(Point<S, 3> const& pt)
{
   return Point<S, 2>(pt.z, pt.y);
}

template <typename S> Cube<S, 2> csg::ProjectOntoXY(Cube<S, 3> const& cube)
{
   return Cube<S, 2>(ProjectOntoXY(cube.min), ProjectOntoXY(cube.max));
}

template <typename S> Cube<S, 2> csg::ProjectOntoXZ(Cube<S, 3> const& cube)
{
   return Cube<S, 2>(ProjectOntoXZ(cube.min), ProjectOntoXZ(cube.max));
}

template <typename S> Cube<S, 2> csg::ProjectOntoYZ(Cube<S, 3> const& cube)
{
   return Cube<S, 2>(ProjectOntoYZ(cube.min), ProjectOntoYZ(cube.max));
}


#define MAKE_CHUNK_TEMPLATES(N) \
   template inline Point3 csg::GetChunkIndex<N>(Point3 const& value); \
   template inline void csg::GetChunkIndex<N>(Point3 const& value, Point3& index, Point3& offset); \
   template inline Cube3 csg::GetChunkIndex<N>(Cube3 const& value); \
   template inline bool csg::PartitionCubeIntoChunks<N>(Cube3 const& cube, std::function<bool(Point3 const& index, Cube3 const& cube)> cb);

MAKE_CHUNK_TEMPLATES(16)
MAKE_CHUNK_TEMPLATES(32)
MAKE_CHUNK_TEMPLATES(64)
MAKE_CHUNK_TEMPLATES(128)

template Region3 csg::GetAdjacent(Region3 const& r, bool allow_diagonals);
template Region3f csg::GetAdjacent(Region3f const& r, bool allow_diagonals);
template Point2 csg::ProjectOntoXY(Point3 const&);
template Point2 csg::ProjectOntoXZ(Point3 const&);
template Point2 csg::ProjectOntoYZ(Point3 const&);
template Rect2 csg::ProjectOntoXY(Cube3 const&);
template Rect2 csg::ProjectOntoXZ(Cube3 const&);
template Rect2 csg::ProjectOntoYZ(Cube3 const&);