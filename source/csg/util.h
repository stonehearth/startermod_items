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

Region3 GetAdjacent(Region3 const& r, bool allow_diagonals, int min_y, int max_y);
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

template <int C> Point<float, C> const& ToFloat(Point<float, C> const& pt);
template <int C> Cube<float, C> const& ToFloat(Cube<float, C> const& cube);
template <int C> Region<float, C> const& ToFloat(Region<float, C> const& region);
template <int C> Point<float, C> ToFloat(Point<int, C> const& pt);
template <int C> Cube<float, C> ToFloat(Cube<int, C> const& cube);
template <int C> Region<float, C> ToFloat(Region<int, C> const& region);
template <int C> Point<int, C> ToInt(Point<float, C> const& pt);
template <int C> Point<int, C> const& ToInt(Point<int, C> const& pt);
template <int C> Cube<int, C> const& ToInt(Cube<int, C> const& cube);
template <int C> Region<int, C> const& ToInt(Region<int, C> const& region);
template <int C> Cube<int, C> ToInt(Cube<float, C> const& cube);
template <int C> Region<int, C> ToInt(Region<float, C> const& pt);
template <typename S> S Interpolate(S const& p0, S const& p1, float alpha);

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

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_UTIL_H
