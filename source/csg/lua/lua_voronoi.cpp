#include "pch.h"
#include "lua/register.h"
#include "lua_voronoi.h"
#include "csg/voronoi_map.h"
#include "csg/heightmap.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;


static void DistributeByHeightmap(VoronoiMap& m, HeightMap<double>& radii, double radius_scale)
{
   LOG(WARNING) << "map starting with " << m.sites.size() << " sites.";

   typedef std::unordered_map<int, std::vector<VoronoiMapSitePtr>> Matrix;
   Matrix buckets;

#define MAKE_KEY(x, y)     (((y) << 16) | (x))

   static const std::pair<int, int> border[] = {
      std::pair<int, int>(-1, -1),
      std::pair<int, int>(-1,  0),
      std::pair<int, int>(-1,  1),
      std::pair<int, int>( 0, -1),
      std::pair<int, int>( 0,  1),
      std::pair<int, int>( 1, -1),
      std::pair<int, int>( 1,  0),
      std::pair<int, int>( 1,  1),
   };

   int size = radii.get_size();
   for (VoronoiMapSitePtr s : m.sites) {
      int x = s->pos.x / radii.get_scale();
      int y = s->pos.y / radii.get_scale();
      double value = radii.get(x, y);
      if (value > 0) {
         s->tag = static_cast<int>(value + 0.5f);
         buckets[MAKE_KEY(x, y)].push_back(s);
      }
   }

   auto prune = [&](VoronoiMapSitePtr s, std::vector<VoronoiMapSitePtr>& buckets, double r2) {
      int i = 0, c = buckets.size();
      while (i < c) {
         VoronoiMapSitePtr t = buckets[i];
         if (s != t) {
            int dx = s->pos.x - t->pos.x;
            int dy = s->pos.y - t->pos.y;
            if ((dx*dx) + (dy*dy) < r2) {
               buckets[i] = buckets[--c];
               continue;
            }
         }
         i++;
      }
      buckets.resize(c);
   };

   // prune all things!
   for (auto &entry : buckets) {
      int x = entry.first & 0xffff;
      int y = entry.first >> 16;
      double radius = radii.get(x, y) * radius_scale;

      // LOG(WARNING) << "pruning " << x << " " << y;

      // clean this bucket...
      std::vector<VoronoiMapSitePtr> &nodes = entry.second;
      for (uint i = 0; i < nodes.size(); i++) {
         prune(nodes[i], nodes, radius * radius);
      }

      // clean adjacent buckets
      for (const auto& delta : border) {
         auto l = buckets.find(MAKE_KEY(x + delta.first, y + delta.second));
         if (l != buckets.end()) {
            for (uint i = 0; i < nodes.size(); i++) {
               prune(nodes[i], l->second, radius * radius);
            }
         }
      }
   }
   
   // copy back
   m.sites.clear();
   for (const auto& entry : buckets) {
      m.sites.insert(m.sites.end(), entry.second.begin(), entry.second.end());
   }
   LOG(WARNING) << "map now has " << m.sites.size() << " sites.";

#undef MAKE_KEY
}

scope LuaVoronoi::RegisterLuaTypes(lua_State* L)
{
   using namespace luabind;
   return
      lua::RegisterTypePtr<VoronoiMapSite>()
         .def(tostring(const_self))
         .def_readonly("pos",    &VoronoiMapSite::pos)
         .def_readonly("verts",  &VoronoiMapSite::verts, return_stl_iterator)
         .def_readwrite("tag",   &VoronoiMapSite::tag)
      ,
      lua::RegisterTypePtr<VoronoiMap>()
         .def(tostring(const_self))
         .def(constructor<int, int>())
         .def_readonly("sites",   &VoronoiMap::sites, return_stl_iterator)
         .def("add_point",        &VoronoiMap::add_point)
         .def("get_adjacent",     &VoronoiMap::get_adjacent)
         .def("generate",         &VoronoiMap::generate)
         .def("distribute_by_heightmap", &DistributeByHeightmap)
      ;
}

