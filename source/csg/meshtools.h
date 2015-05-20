#ifndef _RADIANT_CSG_MESHTOOLS_H
#define _RADIANT_CSG_MESHTOOLS_H

#include <vector>
#include "util.h"
#include "color.h"
#include "region.h"
#include "core/static_string.h"

struct VoxelGeometryVertex;

BEGIN_RADIANT_CSG_NAMESPACE

typedef std::unordered_map<int, Color4> TagToColorMap;

// xxx: must match definition of VoxelGeometryVertex, exactly
struct Vertex {
   float     location[3];
   float     boneIndex;
   float     normal[3];
   float     color[4];

   Vertex(Point3f const&p, Point3f const&n, Point4f const& c);
   Vertex(Point3f const&p, Point3f const&n, Color3 const& c);
   Vertex(Point3f const&p, Point3f const&n, Color4 const& c);
   Vertex(Point3f const& p, float bIndex, Point3f const& n, Point4f const& c);
   Vertex(Point3f const& p, float bIndex, Point3f const& n, Color3 const& c);
   Vertex(Point3f const& p, float bIndex, Point3f const& n, Color4 const& c);

   void SetLocation(Point3f const& p);
   void SetNormal(Point3f const& n);
   void SetColor(Color4 const& c);

   csg::Point3f GetLocation() const;
   csg::Point3f GetNormal() const;
   csg::Color4 GetColor() const;

   bool operator==(Vertex const& other) const;

   struct Hash { 
      inline size_t operator()(Vertex const& p) const;
   };

};

struct Mesh {
   Cube3f               bounds;

   struct Buffers {
      VoxelGeometryVertex const* vertices;
      uint                       vertexCount;

      unsigned int const*        indices;
      uint                       indexCount;
   };

   Mesh();

   Mesh& Clear();
   Mesh& SetOffset(csg::Point3f const& offset);
   Mesh& SetColor(csg::Color4 const& color);
   Mesh& SetColorMap(TagToColorMap const* colorMap);

   bool IsEmpty() const;
   Buffers GetBuffers();

   // This is the one, true add face.  move over to it...
   template <class S> void AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& pi, uint32 boneIndex=0);
   template <class S> void AddRect(Cube<S, 2> const& region, PlaneInfo<S, 3> const& pi, uint32 boneIndex=0);
   void AddFace(Point3f const points[], Point3f const& normal, uint32 boneIndex=0);
   int32 AddVertex(Vertex const& v);
   void AddIndex(int32 index);

private:
   void AddFace(Point3f const points[], Point3f const& normal, Color4 const& color, uint32 boneIndex);
   void SplitTriangle(uint i, uint a0, uint a1, uint a2, uint b0, uint b1, uint b2);
   bool SplitTJunction(csg::Point3f const& point, uint triangle);
   bool PointOnLineSegement(csg::Point3f const& point, uint e0, uint e1);
   uint CreateVertex(csg::Point3f const& point, uint copyFrom);
   void EliminateTJunctures();

private:
   std::vector<Vertex>  _vertices;
   std::vector<int32>   _indices;
   csg::Color4          color_;
   TagToColorMap const* colorMap_;
   bool                 override_color_;
   bool                 flip_;
   Point3f              offset_;
   bool                 _compiled;
   uint                 _duplicatedVertexCount;
   std::unordered_map<Vertex, int32, Vertex::Hash> _vertexMap;
};

// The material name refers to the horde material used to render the mesh.  We alias it here
// to improve the readibility of code using it.
typedef core::StaticString MaterialName;

// The ColorToMaterialMap maps colors to materials which should be used to render them.
typedef std::map<csg::Color3, MaterialName> ColorToMaterialMap;

// The MaterialToMeshMap maps materials to Meshs.  It's often used as a container in the
// intermediate step of splitting a single csg::Region into it's different parts by color
// before meshing each one with a different material.
typedef std::unordered_map<MaterialName, csg::Mesh, MaterialName::Hash> MaterialToMeshMap;

void RegionToMesh(csg::Region3 const& region, Mesh &Mesh, csg::Point3f const& offset, bool optimizePlanes, uint32 boneIndex=0);
void RegionToMeshMap(csg::Region3 const& region, MaterialToMeshMap& meshes, csg::Point3f const& offset, ColorToMaterialMap const& colormap, MaterialName defaultMaterial, uint32 boneIndex=0);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_MESHTOOLS_H
