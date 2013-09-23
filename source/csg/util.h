#ifndef _RADIANT_CSG_UTIL_H
#define _RADIANT_CSG_UTIL_H

#include "region.h"
#include "heightmap.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct EdgePoint;
struct Edge;
struct EdgeList;
typedef std::shared_ptr<EdgePoint> EdgePointPtr;
typedef std::shared_ptr<Edge> EdgePtr;
typedef std::shared_ptr<EdgeList> EdgeListPtr;

// A point at the endge of an edge.  Shared by all edges in the edge list.
struct EdgePoint : public csg::Point2 {
   EdgePoint() { }
   EdgePoint(int x, int y, csg::Point2 const& n) : csg::Point2(x, y), normals(n) { }

   csg::Point2 normals;  // the accumulated normals of all edges
};

struct Edge {
   EdgePointPtr   start;
   EdgePointPtr   end;
   csg::Point2    normal;

   Edge(EdgePointPtr s, EdgePointPtr e, csg::Point2 const& n);
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

bool Region3Intersects(const Region3& rgn, const csg::Ray3& ray, float& distance);
void HeightmapToRegion2(HeightMap<double> const& h, Region2& r);
EdgeListPtr Region2ToEdgeList(csg::Region2 const& rgn, int height, csg::Region3 const& clipper);
csg::Region2 EdgeListToRegion2(EdgeListPtr segments, int width, csg::Region2 const* clipper);

std::ostream& operator<<(std::ostream& os, EdgePoint const& f);
std::ostream& operator<<(std::ostream& os, Edge const& f);
std::ostream& operator<<(std::ostream& os, EdgeList const& f);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_UTIL_H
