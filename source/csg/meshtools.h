#ifndef _RADIANT_CSG_MESHTOOLS_H
#define _RADIANT_CSG_MESHTOOLS_H

#include <vector>
#include "util.h"
#include "color.h"
#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

typedef std::unordered_map<int, Color4> TagToColorMap;

// xxx: must match definition of VoxelGeometryVertex, exactly
struct Vertex {
   Point3f   location; // must be at offset 0
   Point3f   normal;
   Point4f   color;
   Vertex(Point3f const& p, Point3f const& n, Point4f const& c) : location(p), normal(n), color(c) { }
   Vertex(Point3f const& p, Point3f const& n, Color3 const& c) : location(p), normal(n), color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f) { }
   Vertex(Point3f const& p, Point3f const& n, Color4 const& c) : location(p), normal(n), color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f) { }
};

struct Mesh {
   std::vector<Vertex>  vertices;
   std::vector<int32>   indices;
   Cube3f               bounds;

   Mesh();

   Mesh& SetOffset(csg::Point3f const& offset);
   Mesh& SetColor(csg::Color4 const& color);
   Mesh& SetColorMap(TagToColorMap const* colorMap);
   Mesh& FlipFaces();
   // This is the one, true add face.  move over to it...
   template <class S> void AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& pi);
   template <class S> void AddRect(Cube<S, 2> const& region, PlaneInfo<S, 3> const& pi);
   void AddFace(Point3f const points[], Point3f const& normal);

private:
   void AddFace(Point3f const points[], Point3f const& normal, Color4 const& color);

private:
   csg::Color4          color_;
   TagToColorMap const* colorMap_;
   bool                 override_color_;
   bool                 flip_;
   Point3f              offset_;
};

void RegionToMesh(csg::Region3 const& region, Mesh &Mesh, csg::Point3f const& offset, bool optimizePlanes);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_MESHTOOLS_H
