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

#ifndef _RADIANT_CSG_VORONOI_MAP_H
#define _RADIANT_CSG_VORONOI_MAP_H

#include "namespace.h"
#include "point.h"
#include <stdlib.h>
#include <vector>
#include <list>

BEGIN_RADIANT_CSG_NAMESPACE

class VoronoiMapSite {
public:
   Point2 pos;
   std::list<Point2> verts;
   int tag;

   VoronoiMapSite() : tag(0), pos(0, 0) { }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   VoronoiMapSite(const Point2 pos) : pos(pos) {};

   Point2 center();
};

typedef std::shared_ptr<VoronoiMapSite> VoronoiMapSitePtr;
typedef std::vector<VoronoiMapSitePtr> VoronoiMapSiteVector;

class VoronoiMap {
public:
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   VoronoiMap(int width, int height);

   void add_point(int x, int y);
   void generate(int iterations);
   void pprint(std::ostream& os);
   VoronoiMapSiteVector get_adjacent(VoronoiMapSitePtr site);

   VoronoiMapSiteVector sites;
private:
   int width, height;
   std::set<csg::Point2> points;

   void generate_diagram();
   NO_COPY_CONSTRUCTOR(VoronoiMap);
};

std::ostream& operator<<(std::ostream& os, const VoronoiMap& in);
std::ostream& operator<<(std::ostream& os, const VoronoiMapSite& in);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_VORONOI_MAP_H
