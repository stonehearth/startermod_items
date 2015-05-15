#include "pch.h"
#include "color.h"
#include "meshtools.h"
#include "region_tools.h"
#include "region_tools_traits.h"
#include "core/config.h"
#include "core/profiler.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define MT_LOG(level)      LOG(csg.meshtools, level)

typedef std::unordered_map<MaterialName, csg::Region2, MaterialName::Hash> MaterialToPlaneMap;

Vertex::Vertex(Point3f const& p, float bIndex, Point3f const& n, Point4f const& c)
{
   SetLocation(p);
   boneIndex = bIndex;
   SetNormal(n);
   color[0] = (float)c[0];
   color[1] = (float)c[1];
   color[2] = (float)c[2];
   color[3] = (float)c[3];
}

Vertex::Vertex(Point3f const& p, float bIndex, Point3f const& n, Color3 const& c)
{
   SetLocation(p);
   boneIndex = bIndex;
   SetNormal(n);
   color[1] = (float)(c.r / 255.0);
   color[2] = (float)(c.g / 255.0);
   color[3] = (float)(c.b / 255.0);
   color[4] = 1;
}

Vertex::Vertex(Point3f const& p, float bIndex, Point3f const& n, Color4 const& c)
{
   SetLocation(p);
   boneIndex = bIndex;
   SetNormal(n);
   SetColor(c);
}

Vertex::Vertex(Point3f const& p, Point3f const& n, Color4 const& c)
{
   SetLocation(p);
   boneIndex = 0;
   SetNormal(n);
   SetColor(c);
}

Vertex::Vertex(Point3f const& p, Point3f const& n, Color3 const& c)
{
   SetLocation(p);
   boneIndex = 0;
   SetNormal(n);
   color[1] = (float)(c.r / 255.0);
   color[2] = (float)(c.g / 255.0);
   color[3] = (float)(c.b / 255.0);
   color[4] = 1;
}

Vertex::Vertex(Point3f const& p, Point3f const& n, Point4f const& c)
{
   SetLocation(p);
   boneIndex = 0;
   SetNormal(n);
   color[0] = (float)c[0];
   color[1] = (float)c[1];
   color[2] = (float)c[2];
   color[3] = (float)c[3];
}

void Vertex::SetLocation(Point3f const& p)
{
   location[0] = (float)p.x;
   location[1] = (float)p.y;
   location[2] = (float)p.z;
}

void Vertex::SetNormal(Point3f const& n)
{
   normal[0] = (float)n.x;
   normal[1] = (float)n.y;
   normal[2] = (float)n.z;
}

void Vertex::SetColor(Color4 const& c)
{
   color[0] = (float)(c.r / 255.0);
   color[1] = (float)(c.g / 255.0);
   color[2] = (float)(c.b / 255.0);
   color[3] = (float)(c.a / 255.0);
}

csg::Point3f Vertex::GetLocation() const
{
   return csg::Point3f(location[0], location[1], location[2]);
}

csg::Point3f Vertex::GetNormal() const
{
   return csg::Point3f(normal[0], normal[1], normal[2]);
}

csg::Color4 Vertex::GetColor() const
{
   return csg::Color4((unsigned char)(color[0] * 255.0), 
      (unsigned char)(color[1] * 255.0),
      (unsigned char)(color[2] * 255.0),
      (unsigned char)(color[3] * 255.0));
}

bool Vertex::operator==(Vertex const& other) const
{
   return location[0] == other.location[0] &&
          location[1] == other.location[1] &&
          location[2] == other.location[2] &&
          boneIndex == other.boneIndex && 
          normal[0] == other.normal[0] &&
          normal[1] == other.normal[1] &&
          normal[2] == other.normal[2] &&
          color[0] == other.color[0] &&
          color[1] == other.color[1] &&
          color[2] == other.color[2] &&
          color[3] == other.color[3];
}

size_t Vertex::Hash::operator()(Vertex const& p) const
{
   return std::hash<float>()(p.location[0]) ^
            std::hash<float>()(p.location[1]) ^
            std::hash<float>()(p.location[2]) ^
            std::hash<float>()(p.boneIndex) ^
            std::hash<float>()(p.normal[0]) ^
            std::hash<float>()(p.normal[1]) ^
            std::hash<float>()(p.normal[2]) ^
            std::hash<float>()(p.color[0]) ^
            std::hash<float>()(p.color[1]) ^
            std::hash<float>()(p.color[2]) ^
            std::hash<float>()(p.color[3]);
}

int32 Mesh::AddVertex(Vertex const& v)
{
   _compiled = false;

   auto i = _vertexMap.find(v);
   if (i != _vertexMap.end()) {
      ++_duplicatedVertexCount;
      return i->second;
   }
   int32 index = _vertices.size();
   _vertexMap[v] = index;
   _vertices.emplace_back(v);

   return index;
}

void Mesh::AddIndex(int32 index)
{
   _indices.push_back(index);
}

void Mesh::AddFace(Point3f const points[], Point3f const& normal, uint32 boneIndex)
{
   AddFace(points, normal, color_, boneIndex);
}

void Mesh::AddFace(Point3f const points[], Point3f const& normal, Color4 const& c, uint32 boneIndex)
{
   csg::Point4f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);

   float polyOffsetFactor = boneIndex > 0 ? 1.0f - (boneIndex / 10000.0f) : 1.0f;

   if (_vertices.empty()) {
      bounds.SetMin((points[0] * polyOffsetFactor) + offset_);
      bounds.SetMax((points[0] * polyOffsetFactor) + offset_);
   }
   int vlast = static_cast<int>(_vertices.size());
   int32 indices[4];
   for (int i = 0; i < 4; i++) {
      Point3f offsetPoint = (points[i] * polyOffsetFactor) + offset_;
      indices[i] = AddVertex(Vertex(offsetPoint, (float)boneIndex, normal, color));
      bounds.Grow(offsetPoint);
   }
   
   //  it's up to the caller to get the winding right!
   _indices.push_back(indices[0]);  // first triangle...
   _indices.push_back(indices[1]);
   _indices.push_back(indices[2]);

   _indices.push_back(indices[0]);  // second triangle...
   _indices.push_back(indices[2]);
   _indices.push_back(indices[3]);
}

template <typename S>
void Mesh::AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& p, uint32 boneIndex)
{
   for (Cube<S, 2> const& rect : EachCube(region)) {
      AddRect(rect, p, boneIndex);
   }
}

template <class S>
void Mesh::AddRect(Cube<S, 2> const& rect, PlaneInfo<S, 3> const& p, uint32 boneIndex)
{
   PlaneInfo3f pi = ToFloat(p);
   Point3f normal = pi.GetNormal();

   Point2f p0 = ToFloat(rect.min);
   Point2f p2 = ToFloat(rect.max);

   // In a rect min < max for all coords in the point.  To get the winding
   // correct, we need to re-order points depending on the direction of the
   // normal
   bool reverse_winding = (pi.reduced_coord == 2) ? (pi.normal_dir < 0) : (pi.normal_dir > 0);
   if (reverse_winding) {
      std::swap(p0, p2);
   }
   Point2f p1 = ToFloat(Point<S, 2>(rect.max.x, rect.min.y));
   Point2f p3 = ToFloat(Point<S, 2>(rect.min.x, rect.max.y));

   Point3f points[4] = {
      RegionToolsTraits3f::ExpandPoint(p0, pi),
      RegionToolsTraits3f::ExpandPoint(p1, pi),
      RegionToolsTraits3f::ExpandPoint(p2, pi),
      RegionToolsTraits3f::ExpandPoint(p3, pi),
   };

   int tag = rect.GetTag();
   csg::Color4 color = Color4::FromInteger(tag);

   if (override_color_) {
      color = color_;      
   } else if (colorMap_) {
      auto i = colorMap_->find(tag);
      if (i != colorMap_->end()) {
         color = i->second;
      }
   }
   AddFace(points, normal, color, boneIndex);
}

Mesh::Mesh() : 
   override_color_(false),
   color_(0, 0, 0, 255),
   colorMap_(nullptr),
   offset_(-0.5f, 0.0f, -0.5f),
   _compiled(false),
   _duplicatedVertexCount(0)
{
}

bool Mesh::IsEmpty() const
{
   return _vertices.size() == 0;
}

Mesh& Mesh::SetColor(csg::Color4 const& color)
{
   color_ = csg::Color4(color.r, color.g, color.b, color.a);
   override_color_ = true;
   return *this;
}

Mesh& Mesh::SetColorMap(TagToColorMap const* colorMap)
{
   colorMap_ = colorMap;
   return *this;
}

Mesh& Mesh::SetOffset(csg::Point3f const& offset)
{
   offset_ = offset;
   return *this;
}

Mesh& Mesh::Clear()
{
   _vertices.clear();
   _indices.clear();
   bounds = Cube3f::zero;
   return *this;
}

void csg::RegionToMesh(csg::Region3 const& region, Mesh &mesh, csg::Point3f const& offset, bool optimizePlanes, uint32 boneIndex)
{
   mesh.SetOffset(offset);
   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      if (optimizePlanes) {
         csg::Region2 optPlane = plane;
         optPlane.ForceOptimizeByMerge("converting region to mesh plane");
         mesh.AddRegion(optPlane, pi, boneIndex);
      } else {
         mesh.AddRegion(plane, pi, boneIndex);
      }
   });
}

//
// -- csg::RegionToMeshMap
//
// Convert `region` to a series of meshes, separated by material name.
//
void csg::RegionToMeshMap(csg::Region3 const& region, MaterialToMeshMap& meshes, csg::Point3f const& offset, ColorToMaterialMap const& colormap, MaterialName defaultMaterial, uint32 boneIndex)
{
   auto colormapEnd = colormap.end();

   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      MaterialToPlaneMap planesByMaterial;

      // Look up the material for every cube in the plane, accumulating into a Region2 for each
      // material.
      for (csg::Rect2 const& r : csg::EachCube(plane)) {
         csg::Color3 color = csg::Color3::FromInteger(r.GetTag());
         auto i = colormap.find(color);
         MaterialName material = (i == colormapEnd) ? defaultMaterial : i->second;
         planesByMaterial[material].AddUnique(r);
      }

      // Optimize the accumulated planes and throw them into the proper mesh, by material
      for (auto& entry : planesByMaterial) {
         MaterialName name = entry.first;
         csg::Region2& plane = entry.second;
         plane.ForceOptimizeByMerge("meshing by material");

         csg::Mesh& mesh = meshes[name];
         mesh.SetOffset(offset);
         mesh.AddRegion(plane, pi, boneIndex);
      }
   });
}

#define LOG_TRIANGLE(i) " tri(" << LOG_INDEX_POINT(i) << ", " << LOG_INDEX_POINT(i + 1) << ", " << LOG_INDEX_POINT(i + 2) << ")";
#define LOG_INDEX_POINT(i)    "(ii:" << i << " vi:" << _indices[i] << " pt:" << _vertices[_indices[i]].GetLocation() << " n:" << _vertices[_indices[i]].GetNormal() << ")"
#define LOG_VERTEX_POINT(vi)  "(vi:" << vi << " pt:" << _vertices[vi].GetLocation() << " n:" << _vertices[vi].GetNormal() << ")"

#define CHECK_BETWEEN_COMP(pt, p0, p1, coord) \
   if (p0.coord <= p1.coord) { \
      if (pt.coord < p0.coord || pt.coord > p1.coord) { \
         return false; \
      } \
   } else { \
      if (pt.coord < p1.coord || pt.coord > p0.coord) { \
         return false; \
      } \
   }

#define CHECK_BETWEEN(pt, v0, v1) \
   CHECK_BETWEEN_COMP(pt, v0, v1, x) \
   CHECK_BETWEEN_COMP(pt, v0, v1, y) \
   CHECK_BETWEEN_COMP(pt, v0, v1, z)

bool Mesh::PointOnLineSegement(csg::Point3f const& point, uint ei0, uint ei1)
{
   Point3f edge0 = _vertices[_indices[ei0]].GetLocation();
   Point3f edge1 = _vertices[_indices[ei1]].GetLocation();

   // reject quickly...
   CHECK_BETWEEN(point, edge0, edge1);

   double d1 = edge0.DistanceTo(point);
   if (d1 == 0) {
      LOG(csg.meshtools, 9) << "edge0 == vertex.  bailing";
      return false;
   }
   
   double d2 = edge1.DistanceTo(point);
   if (d2 == 0) {
      LOG(csg.meshtools, 9) << "edge1 == vertex.  bailing";
      return false;
   }

   double d0 = edge0.DistanceTo(edge1);
   bool onLine = fabs(d0 - (d1 + d2)) < 0.001;

   LOG(csg.meshtools, 9) << "fabs( d0(" << d0 << ") - (d1(" << d1 << ") + d2(" << d2 << ")) ) = " << fabs(d0 - (d1 + d2));

   if (onLine) {
      LOG(csg.meshtools, 7) << "found t-junction @ " << point
                            << " between " << LOG_INDEX_POINT(ei0)
                            << " and " << LOG_INDEX_POINT(ei1) << ".";
   }
   return onLine;
}

uint Mesh::CreateVertex(csg::Point3f const& point, uint copyFrom)
{
   Vertex newVertex(_vertices[_indices[copyFrom]]);        // copy the rest from the current edge.

   newVertex.location[0] = (float)point.x;
   newVertex.location[1] = (float)point.y;
   newVertex.location[2] = (float)point.z;

   uint nv = (uint)AddVertex(newVertex);
   LOG(csg.meshtools, 9) << "created new vertex " << LOG_VERTEX_POINT(nv);

   return nv;
}

void Mesh::SplitTriangle(uint i, uint a0, uint a1, uint a2, uint b0, uint b1, uint b2)
{
   _indices[i]   = a0;
   _indices[i+1] = a1;
   _indices[i+2] = a2;

   uint newi = _indices.size();
   _indices.push_back(b0);
   _indices.push_back(b1);
   _indices.push_back(b2);

   LOG(csg.meshtools, 9) << "modified existing triangle " << LOG_TRIANGLE(i);
   LOG(csg.meshtools, 7) << "created new triangle " << LOG_TRIANGLE(newi);         
}

bool Mesh::SplitTJunction(csg::Point3f const& vi, uint triangle)
{
   LOG(csg.meshtools, 9) << "testing vertex " << vi << " against " << LOG_TRIANGLE(triangle);

   uint ti0 = triangle;
   uint ti1 = triangle + 1;
   uint ti2 = triangle + 2;

   if (PointOnLineSegement(vi, ti0, ti1)) {
      uint nvi = CreateVertex(vi, ti0);
      // create 2 new triangles (ti0, nvi, ti2) and (nvi, ti1, ti2)
      LOG(csg.meshtools, 9) << "splitting edge 0 1";
      SplitTriangle(ti0, _indices[ti0], nvi, _indices[ti2],
                         nvi, _indices[ti1], _indices[ti2]);
      return true;
   }

   if (PointOnLineSegement(vi, ti1, ti2)) {
      uint nvi = CreateVertex(vi, ti0);
      // create 2 new triangles (ti0, nvi, ti2) (ti0, ti1, nvi)
      LOG(csg.meshtools, 9) << "splitting edge 1 2";
      SplitTriangle(ti0, _indices[ti0], nvi, _indices[ti2],
                         _indices[ti0], _indices[ti1], nvi);
      return true;
   }

   if (PointOnLineSegement(vi, ti2, ti0)) {
      uint nvi = CreateVertex(vi, ti0);
      // create 2 new triangles (ti0, ti1, nvi) (nvi, ti1, ti2)
      LOG(csg.meshtools, 9) << "splitting edge 2 0";
      SplitTriangle(ti0, _indices[ti0], _indices[ti1], nvi,
                         nvi, _indices[ti1], _indices[ti2]);
      return true;
   }
   return false;
}

Mesh::Buffers Mesh::GetBuffers() 
{
   if (!_compiled) {
      EliminateTJunctures();

      float saved = 100.0f * _duplicatedVertexCount / (_duplicatedVertexCount + _vertices.size());
      LOG(csg.meshtools, 3) << std::fixed << std::setprecision(2) << saved << "% of _vertices were duplicates (total:" << _vertices.size() << ")";
      _compiled = true;
   }

   Mesh::Buffers buffers;
   buffers.vertices = (VoxelGeometryVertex const*)_vertices.data();
   buffers.vertexCount = _vertices.size();

   buffers.indices = (unsigned int const*)_indices.data();
   buffers.indexCount = _indices.size();

   return buffers;
}

void Mesh::EliminateTJunctures()
{
   if (!core::Config::GetInstance().Get<bool>("enable_t_junction_removal", true)) {
      return;
   }
   core::SetProfilerEnabled(true);

   LOG(csg.meshtools, 7) << "eliminating t-junctions... << " << "(_indices:" << _indices.size() << " _vertices:" << _vertices.size() << ")";

   std::vector<csg::Point3f> points;
   std::unordered_set<csg::Point3f, csg::Point3f::Hash> visited;

   // processing a point is expensive, so collapse all the vertices which
   // share a location but have a different color, normal, etc.
   for (auto const& v : _vertices) {
      csg::Point3f p = v.GetLocation();
      if (visited.emplace(p).second) {
         points.push_back(p);
      }
   }

   // run through all the verts looking for intersections with other edges.
   // when we find one, split it.
   for (csg::Point3f const& candidate : points) {
      LOG(csg.meshtools, 5) << "checking triangle starting @ " << candidate << " (_indices:" << _indices.size() << " _vertices:" << _vertices.size() << ")";

      // test all points of the candidate against the edges of the ith triangle.
      for (uint i = 0; i < _indices.size(); i += 3) {
         SplitTJunction(candidate, i);
      }
   }
   core::SetProfilerEnabled(false);

   LOG(csg.meshtools, 5) << "t-junctions eliminated!";
}

#undef LOG_INDEX_POINT

template void Mesh::AddRegion(Region<double, 2> const& region, PlaneInfo<double, 3> const& p, uint32 boneIndex);
template void Mesh::AddRegion(Region<int, 2> const& region, PlaneInfo<int, 3> const& p, uint32 boneIndex);
template void Mesh::AddRect(Cube<double, 2> const& region, PlaneInfo<double, 3> const& p, uint32 boneIndex);
template void Mesh::AddRect(Cube<int, 2> const& region, PlaneInfo<int, 3> const& p, uint32 boneIndex);
