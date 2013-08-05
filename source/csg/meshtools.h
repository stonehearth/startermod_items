#ifndef _RADIANT_CSG_MESHTOOLS_H
#define _RADIANT_CSG_MESHTOOLS_H

#include <vector>
#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

class mesh_tools
{
public:
   // xxx: must match definition of VoxelGeometryVertex, exactly
   struct vertex {
      csg::Point3f   location; // must be at offset 0
      csg::Point3f   normal;
      csg::Point3f   color;
      vertex(csg::Point3f const& p, csg::Point3f const& n, csg::Point3f const& c) : location(p), normal(n), color(c) { }
   };

   struct mesh {
      std::vector<vertex>  vertices;
      std::vector<int32>   indices;
      csg::Cube3f          bounds;

      void add_face(csg::Point3f const points[], csg::Point3f const& normal, csg::Point3f const& color);
   };

   typedef std::unordered_map<int, mesh>  meshmap;
   typedef std::function<void(int, csg::Point3f const [], csg::Point3f const&, csg::mesh_tools::mesh&)> tesselator_fn;
   typedef std::unordered_map<int, tesselator_fn> tesselator_map;

public:
   mesh_tools();
   mesh_tools(tesselator_map const& tessmap);
   void optimize_region(const csg::Region3& region, meshmap& m);

private:
   typedef std::unordered_map<int, csg::Region2> face_map;
   struct plane_data {
      int plane, x, y;
   };
   void add_face_map(face_map const& faces, face_map const& occluders, plane_data const& p, int normal, meshmap& m);
   void add_region(csg::Region2 const& region, int plane_value, plane_data const& p, int normal, meshmap& m);
   void add_face(csg::Point3f const points[], csg::Point3f normal, mesh& mm);

private:
   tesselator_map const&      tesselators_;
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_MESHTOOLS_H
