#ifndef _RADIANT_CSG_EDGE_TOOLS_H
#define _RADIANT_CSG_EDGE_TOOLS_H

#include <vector>
#include "util.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <typename S, int C>
struct EdgeInfo
{
   Point<S, C>    min;
   Point<S, C>    max;
   Point<S, C>    normal;
};

template <typename S, int C>
class EdgePoint
{
public:
   EdgePoint(Point<S, C> const& l, Point<S, C> const& a) : location(l), accumulated_normals(a) { }

   EdgePoint& AccumulateNormal(Point<S, C> const& n) {
      accumulated_normals += n;
      return *this;
   }

   EdgePoint& FixAccumulatedNormal() {
      for (int i = 0; i < C; i++) {
         if (accumulated_normals[i] < 0) {
            accumulated_normals[i] = -1;
         } else if (accumulated_normals[i] > 0) {
            accumulated_normals[i] = 1;
         }
      }
      return *this;
   }

public:
   Point<S, C>    location;
   Point<S, C>    accumulated_normals;
};

template <typename S, int C>
class Edge
{
public:
   Edge(EdgePoint<S, C> const* a, EdgePoint<S, C> const* b, Point<S, C> const& n) : min(a), max(b), normal(n) { }

public:
   EdgePoint<S, C> const*  min;
   EdgePoint<S, C> const*  max;
   Point<S, C>             normal;
};

template <typename S, int C>
class EdgeMap
{
public:
   EdgeMap& AddEdge(Point<S, C> const& min, Point<S, C> const& max, Point<S, C> const& normal) {
      DEBUG_ONLY(
         for (Edge<S, C> const& e : edges) {
            ASSERT(e.min->location != min || e.max->location != max || e.normal != normal);
         }
      )
      edges.emplace_back(Edge<S, C>(AddPoint(min, normal),
                                    AddPoint(max, normal),
                                    normal));
      return *this;
   }
   ~EdgeMap() {
      for (auto point : points) {
         delete point;
      }
   }

   typename std::vector<Edge<S, C>>::const_iterator begin() { return edges.begin(); }
   typename std::vector<Edge<S, C>>::const_iterator end() { return edges.end(); }

   bool FindEdgePoint(Point<S, C> const& pt, EdgePoint<S, C>** edge_point) {
      for (EdgePoint<S, C>* ep : points) {
         if (ep->location == pt) {
            *edge_point = ep;
            return true;
         }
      }
      return false;
   }

   void FixNormals() {
      for (EdgePoint<S, C>* point : points) {
         point->FixAccumulatedNormal();
      }
   }

private:
   EdgePoint<S, C>* AddPoint(Point<S, C> const& p, Point<S, C> const& normal) {
      for (auto& point : points) {
         if (point->location == p) {
            point->AccumulateNormal(normal);
            return point;
         }
      }
      points.emplace_back(new EdgePoint<S, C>(p, normal));
      return points.back();
   }

private:
   std::vector<Edge<S, C>>          edges;
   std::vector<EdgePoint<S, C>*>    points;
};

typedef EdgeMap<int, 2> EdgeMap2;
typedef EdgeMap<int, 3> EdgeMap3;
typedef EdgeInfo<int, 2> EdgeInfo2;
typedef EdgeInfo<int, 3> EdgeInfo3;

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_EDGE_TOOLS_H
