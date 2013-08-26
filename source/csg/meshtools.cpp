#include "pch.h"
#include "color.h"
#include "meshtools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

static mesh_tools::tesselator_map no_tess_map;

mesh_tools::mesh_tools() :
   tesselators_(no_tess_map)
{
}

mesh_tools::mesh_tools(tesselator_map const& tessmap) :
   tesselators_(tessmap)
{
}

void mesh_tools::optimize_region(const csg::Region3& region, meshmap& m)
{   
   static const plane_data plane_data[] = { 
      { 0, 2, 1 },
      { 1, 0, 2 },
      { 2, 0, 1 },
   };

   for (int plane = 0; plane < 3; plane++) {
      auto const& p = plane_data[plane];
      face_map front, back;

      for (csg::Cube3 const& cube : region) {
         csg::Point3 const& min = cube.GetMin();
         csg::Point3 const& max = cube.GetMax();
         csg::Rect2 face(csg::Point2(min[p.x], min[p.y]), csg::Point2(max[p.x], max[p.y]), cube.GetTag());
         front[min[p.plane]].AddUnique(face);
         back[max[p.plane]].AddUnique(face);
      }
      add_face_map(front, back, p, -1, m);
      add_face_map(back, front, p,  1, m);
   }
}

void mesh_tools::add_face_map(face_map const& faces, face_map const& occluders, plane_data const& p, int normal, meshmap& m)
{
   for (auto const& entry : faces) {
      auto i = occluders.find(entry.first);
      if (i == occluders.end()) {
         add_region(entry.second, entry.first, p, normal, m);
      } else {
         add_region(entry.second - i->second, entry.first, p, normal, m);
      }
   } 
}

void mesh_tools::add_region(csg::Region2 const& region, int plane_value, plane_data const& p, int normal_dir, meshmap& mm)
{
   for (csg::Rect2 const& r: region) {
      csg::Point2 const& min = r.GetMin();
      csg::Point2 const& max = r.GetMax();

      csg::Point3f points[4];
      points[0][p.x] = (float)min.x;
      points[0][p.y] = (float)min.y;
      points[0][p.plane] = (float)plane_value;

      points[1][p.x] = (float)min.x;
      points[1][p.y] = (float)max.y;
      points[1][p.plane] = (float)plane_value;

      points[2][p.x] = (float)max.x;
      points[2][p.y] = (float)max.y;
      points[2][p.plane] = (float)plane_value;

      points[3][p.x] = (float)max.x;
      points[3][p.y] = (float)min.y;
      points[3][p.plane] = (float)plane_value;

      csg::Point3f normal(0, 0, 0);
      normal[p.plane] = (float)normal_dir;

      int tag = r.GetTag();
      mesh& m = mm[tag];

      // this is super gross.  instead of passing in a tess map, we should parameterize the 
      // mesh_tools object with a function that we can call...  or better yet, dump this
      // thing and do tesselation on the server!
      if (tesselators_.empty()) {         
         csg::Color3 c= csg::Color3::FromInteger(tag);
         csg::Point3f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
         m.add_face(points, normal, color);
      } else {
         auto i = tesselators_.find(tag);
         if (i != tesselators_.end()) {
            i->second(tag, points, normal, m);
         } else {
            m.add_face(points, normal, csg::Point3f(0.5, 0.5, 0.5));
         }
      }
   }
}

void mesh_tools::mesh::add_face(csg::Point3f const points[], csg::Point3f const& normal, csg::Point3f const& color)
{
   if (vertices.empty()) {
      bounds.SetMin(points[0]);
      bounds.SetMax(points[0]);
   }
   for (int i = 0; i < 4; i++) {
      bounds.Grow(points[i]);
   }

   int vlast = vertices.size();
   for (int i = 0; i < 4; i++) {
      vertices.emplace_back(vertex(points[i], normal, color));
   }

   if (normal.x < 0 || normal.y < 0 || normal.z > 0) {
      indices.push_back(vlast + 2);  // first triangle...
      indices.push_back(vlast + 1);
      indices.push_back(vlast + 0);

      indices.push_back(vlast + 3);  // second triangle...
      indices.push_back(vlast + 2);
      indices.push_back(vlast + 0);
   } else {
      indices.push_back(vlast + 0);  // first triangle...
      indices.push_back(vlast + 1);
      indices.push_back(vlast + 2);

      indices.push_back(vlast + 0);  // second triangle...
      indices.push_back(vlast + 2);
      indices.push_back(vlast + 3);
   }
}
