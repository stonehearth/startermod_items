#ifndef _RADIANT_CSG_UTIL_H
#define _RADIANT_CSG_UTIL_H

#include "region.h"
#include "heightmap.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct EdgePointX;
struct EdgeX;
struct EdgeList;
typedef std::shared_ptr<EdgePointX> EdgePointPtr;
typedef std::shared_ptr<EdgeX> EdgePtr;
typedef std::shared_ptr<EdgeList> EdgeListPtr;

// A point at the endge of an edge.  Shared by all edges in the edge list.
struct EdgePointX : public csg::Point2 {
   EdgePointX() { }
   EdgePointX(int x, int y, csg::Point2 const& n) : csg::Point2(x, y), normals(n) { }

   csg::Point2 normals;  // the accumulated normals of all edges
};

struct EdgeX {
   EdgePointPtr   start;
   EdgePointPtr   end;
   csg::Point2    normal;

   EdgeX(EdgePointPtr s, EdgePointPtr e, csg::Point2 const& n);
};

struct EdgeList {
   std::vector<EdgePointPtr>  points;
   std::vector<EdgePtr>       edges;

   void Inset(int dist);
   void Grow(int dist);
   void Fragment();
   void AddEdge(csg::Point2 const& start, csg::Point2 const& end, csg::Point2 const& normal);

private:
   EdgePointPtr GetPoint(csg::Point2 const& pt, csg::Point2 const& normal);
};

// Slow versions which resort to std::floor().  ew!
int GetChunkAddressSlow(int value, int chunk_width);
int GetChunkIndexSlow(int value, int chunk_width);
void GetChunkIndexSlow(int value, int chunk_width, int& index, int& offset);
Point3 GetChunkIndexSlow(Point3 const& value, int chunk_width);
void GetChunkIndexSlow(Point3 const& value, int chunk_width, Point3& index, Point3& offset);
Cube3 GetChunkIndexSlow(Cube3 const& value, int chunk_width);
bool PartitionCubeIntoChunksSlow(Cube3 const& cube, int width, std::function<bool(Point3 const& index, Cube3 const& cube)> const& cb);

// Fast versions which use templates.  Prefer these when the size is known at compile time, especially when using power of 2 
// widths.
template <int S> int GetChunkAddress(int value);
template <int S> int GetChunkIndex(int value);
template <int S> void GetChunkIndex(int value, int& index, int& offset);
template <int S> Point3 GetChunkIndex(Point3 const& value);
template <int S> void GetChunkIndex(Point3 const& value, Point3& index, Point3& offset);
template <int S> Cube3 GetChunkIndex(Cube3 const& value);
template <int S> bool PartitionCubeIntoChunks(Cube3 const& cube, std::function<bool(Point3 const& index, Cube3 const& cube)> cb);

template <typename Region> Region GetAdjacent(Region const& r, bool allow_diagonals);

bool Region3Intersects(const Region3& rgn, const csg::Ray3& ray, float& distance);
void HeightmapToRegion2(HeightMap<double> const& h, Region2& r);
EdgeListPtr Region2ToEdgeList(csg::Region2 const& rgn, int height, csg::Region3 const& clipper);
csg::Region2 EdgeListToRegion2(EdgeListPtr segments, int width, csg::Region2 const* clipper);
int RoundTowardNegativeInfinity(int i, int tile_size);
int GetTileOffset(int position, int tile_size);
bool Region3Intersects(const Region3& rgn, const csg::Ray3& ray, float& distance);
Region3 Reface(Region3 const& rgn, Point3 const& forward);
bool Cube3Intersects(const Cube3& rgn, const Ray3& ray, float& distance);
bool Cube3Intersects(const Cube3f& rgn, const Ray3& ray, float& distance);

int ToInt(int i);
int ToInt(float s);
int ToClosestInt(int i);
int ToClosestInt(float s);

template <int C> Point<float, C> const& ToFloat(Point<float, C> const& pt);
template <int C> Cube<float, C> const& ToFloat(Cube<float, C> const& cube);
template <int C> Region<float, C> const& ToFloat(Region<float, C> const& region);
template <int C> Point<float, C> ToFloat(Point<int, C> const& pt);
template <int C> Cube<float, C> ToFloat(Cube<int, C> const& cube);
template <int C> Region<float, C> ToFloat(Region<int, C> const& region);
template <int C> Point<int, C> ToInt(Point<float, C> const& pt);
template <int C> Point<int, C> const& ToInt(Point<int, C> const& pt);
template <int C> Point<int, C> ToClosestInt(Point<float, C> const& pt);
template <int C> Point<int, C> const& ToClosestInt(Point<int, C> const& pt);
template <int C> Cube<int, C> const& ToInt(Cube<int, C> const& cube);
template <int C> Region<int, C> const& ToInt(Region<int, C> const& region);
template <int C> Cube<int, C> ToInt(Cube<float, C> const& cube);
template <int C> Region<int, C> ToInt(Region<float, C> const& pt);
template <typename S> S Interpolate(S const& p0, S const& p1, float alpha);

template <typename T, int C, typename F> Point<T, C> ConvertTo(Point<F, C> const& pt);
template <typename T, int C, typename F> Cube<T, C> ConvertTo(Cube<F, C> const& cube);

template <typename S> static inline bool IsGreaterEqual(S a, S b);
template <typename S> static inline bool IsBetween(S a, S b, S c);

template<> static inline bool IsGreaterEqual(int a, int b) { 
   return a >= b;
}
template<> static inline bool IsBetween(int a, int b, int c) { 
   return a <= b && b < c;
}

template<> static inline bool IsGreaterEqual(float a, float b) { 
   return (a + k_epsilon) >= b;
}

template<> static inline bool IsBetween(float a, float b, float c) {
   return a <= (b + k_epsilon) && b < (c + k_epsilon);
}

std::ostream& operator<<(std::ostream& os, EdgePointX const& f);
std::ostream& operator<<(std::ostream& os, EdgeX const& f);
std::ostream& operator<<(std::ostream& os, EdgeList const& f);


template <int C>
struct ToClosestIntTransform {
   Point<float, C> operator()(Point<float, C> const& key) const {
      return csg::ToFloat(ToClosestInt(key));
   }
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_UTIL_H
