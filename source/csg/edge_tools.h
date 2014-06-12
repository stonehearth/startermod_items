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

template <typename S, int C> class EdgePoint;

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
class EdgePoint
{
public:
   EdgePoint(Point<S, C> const& l, Point<S, C> const& a) : location(l), accumulated_normals(a) { }

   void AccumulateNormal(Point<S, C> const& n) {
      accumulated_normals += n;
   }

   void FixAccumulatedNormal() {
      for (int i = 0; i < C; i++) {
         if (accumulated_normals[i] < 0) {
            accumulated_normals[i] = -1;
         } else if (accumulated_normals[i] > 0) {
            accumulated_normals[i] = 1;
         }
      }
   }

   void CheckForTJunction(Edge<S, C> const& edge) {
      Point<S, C> const& min = edge.min->location;
      Point<S, C> const& max = edge.max->location;

      for (int i = 0; i < C; i++) {
         if (edge.normal[i]) {
            // In the normal direction, the tangent coordinates must match
            ASSERT(min[i] == max[i]);
            if (location[i] != min[i]) {
               return;
            }
         } else {
            // In tangent directions, we need to be between the min
            // and the max (not on them.  between them!)
            if (location[i] <= min[i] || location[i] >= max[i]) {
               return;
            }
         }
      }

      // Whoops!  T-Junction.  Add the normal of the intersecting edge to 
      // our accumated normal.
      AccumulateNormal(edge.normal);
   }

public:
   Point<S, C>    location;
   Point<S, C>    accumulated_normals;
};

template <typename S, int C>
class EdgeMap
{
public:
   EdgeMap()
   {
   }

   EdgeMap(EdgeMap &&other) :
      edges(std::move(other.edges)),
      points(std::move(other.points))
   {
   }

   // Copy constructor.  The copy constructor is slow and gross.  It exists only
   // to facilitate pusing EdgeMaps into lua.
   EdgeMap(EdgeMap &other)
   {
      std::unordered_map<EdgePoint<S, C> const*, EdgePoint<S, C> const*> pointmap;

      for (EdgePoint<S, C>* ep : other.points) {
         EdgePoint<S, C>* p = new EdgePoint<S, C>(*ep);
         points.push_back(p);
         pointmap[ep] = p;
      }

      for (Edge<S, C> const& edge : other.edges) {
         edges.push_back(Edge<S, C>(pointmap[edge.min], pointmap[edge.max], edge.normal));
      }
   }

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
      // T-intersections are a killer.  The top point at the vertical edge of a T-section
      // doesn't have any normal contribution from the top of the T.  It should point diagonally
      // away from the junction, but actually points parallel to the top.  Not good!  Find
      // these junctions and accumulate them into the point.  We do this at the end so we
      // don't have to worry about ordering of when edges are added.

      for (EdgePoint<S, C>* point : points) {
         for (Edge<S, C> const& edge: edges) {
            point->CheckForTJunction(edge);
         }
      }

      // Now normalize the accumualted normals.
      for (EdgePoint<S, C>* point : points) {
         point->FixAccumulatedNormal();
      }
   }

   std::vector<Edge<S, C>> const& GetEdges() const { return edges; }
   std::vector<EdgePoint<S, C>*> const& GetPoints() const { return points; }

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

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_EDGE_TOOLS_H
