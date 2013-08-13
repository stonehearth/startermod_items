/*
* Copyright (c) 2012 by Joseph Marshall
*
* Permission to use, copy, modify, and distribute this software for any
* purpose without fee is hereby granted, provided that this entire notice
* is included in all copies of any software which is or includes a copy
* or modification of this software and in all copies of the supporting
* documentation for such software.
* THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
* REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
* OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
*/

#include "pch.h"
#include "voronoi.h"
#include "voronoi_map.h"
#include "heightmap.h"

using namespace ::radiant;
using namespace ::radiant::csg;

// disable 'possible loss of data' when doing automatic conversions between int/float
#pragma warning( push )
#pragma warning( disable : 4244 )

std::ostream& csg::operator<<(std::ostream& os, const VoronoiMap& in)
{
   return (os << "(VoronoiMap " << in.sites.size() << " sites)");
}

std::ostream& csg::operator<<(std::ostream& os, const VoronoiMapSite& in)
{
   return (os << "(VoronoiMapSite @ " << in.pos << ")");
}

Point2 VoronoiMapSite::center() {
   Point2 center(0, 0);
   for (auto &v : verts) {
      center += v;
   }
   return Point2(static_cast<int>(static_cast<float>(center.x) / verts.size() + 0.5f),
      static_cast<int>(static_cast<float>(center.y) / verts.size() + 0.5f));
}

VoronoiMap::VoronoiMap(int width, int height) :
   width(width), height(height)
{
}

void VoronoiMap::add_point(int x, int y)
{
   points.insert(csg::Point2(x, y));
}


void VoronoiMap::generate(int iterations)
{
   while (true) {
      generate_diagram();
      if (--iterations <= 0) {
         break;
      }
      points.clear();
      for (VoronoiMapSitePtr const& site : sites) {
         points.insert(site->center());
      }
   };
}

void VoronoiMap::generate_diagram()
{
   std::vector<csg::Point2> point_vector(points.begin(), points.end());

   Voronoi v(point_vector, 0, width, 0, height);

   int i = 0;
   sites.resize(points.size());
   for (auto &vsite : v.sites) {
      auto site = sites[i] = std::make_shared<VoronoiMapSite>(point_vector[i]);

      VSitePoints tmp_ = vsite.edges.back();
      Point2 first = tmp_.p0;
      site->verts.push_back(first);
      Point2 last = tmp_.p1;
      if (first != last)
         site->verts.push_back(last);


      vsite.edges.pop_back();

      // Convert edges to a single loop of vertices.
      bool reversed = false;
      int md = 1;
      while (vsite.edges.size() > 0) {
         bool found = false;
         int idx = 0;
         for (auto &e : vsite.edges) {
            Point2 d = e.p0 -last;
            if (abs(d.x) < md && abs(d.y) < md) {
               if (reversed) {
                  if (e.p1 != site->verts.front())
                     site->verts.emplace_front(e.p1);
               } else {
                  if (e.p1 != site->verts.back())
                     site->verts.emplace_back(e.p1);
               }
               last = e.p1;
               found = true;
               break;
            }

            d = e.p1-last;
            if (abs(d.x) < md && abs(d.y) < md) {
               if (reversed) {
                  if (e.p0 != site->verts.front())
                     site->verts.emplace_front(e.p0);
               } else {
                  if (e.p0 != site->verts.back())
                     site->verts.emplace_back(e.p0);
               }
               last = e.p0;
               found = true;
               break;
            }
            idx ++;
         }

         if (found) { 
            vsite.edges.erase(vsite.edges.begin()+idx);
         } else {
            if (reversed) {
               // TODO we shouldn't get dangling edges like this!
               // Here we try to recover from it and snap to the next
               // closest edge point.
               md ++;
               continue;
            }
            last = first;
            reversed = true;
         }
         md = 1;
      }
      if (site->verts.front() == site->verts.back()) {
         site->verts.pop_back();
      }
      i ++;
   }
}

void VoronoiMap::pprint(std::ostream& os) {
   for (auto &s : sites) {
      os << s->pos.x << "," << s->pos.y << std::endl;
   }
   for (auto &s : sites) {
      os << "|" << std::endl;
      for (auto &v : s->verts) {
         os << v.x << "," << v.y << std::endl;
      }
   }
}

VoronoiMapSiteVector VoronoiMap::get_adjacent(VoronoiMapSitePtr site)
{
   VoronoiMapSiteVector result;

   std::set<Point2> site_points(site->verts.begin(), site->verts.end());
   auto end = site_points.end();
   for (VoronoiMapSitePtr s : sites) {
      if (s != site) {
         for (Point2 const& pt : s->verts) {
            if (site_points.find(pt) != end) {
               result.push_back(s);
               break;
            }
         }
      }
   }
   return result;
}


#pragma warning( pop )