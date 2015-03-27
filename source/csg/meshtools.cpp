#include "pch.h"
#include "color.h"
#include "meshtools.h"
#include "region_tools.h"
#include "region_tools_traits.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define MT_LOG(level)      LOG(csg.meshtools, level)

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


void Mesh::AddFace(Point3f const points[], Point3f const& normal, uint32 boneIndex)
{
   AddFace(points, normal, color_, boneIndex);
}

void Mesh::AddFace(Point3f const points[], Point3f const& normal, Color4 const& c, uint32 boneIndex)
{
   csg::Point4f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);

   float polyOffsetFactor = boneIndex > 0 ? 1.0f - (boneIndex / 10000.0f) : 1.0f;

   if (vertices.empty()) {
      bounds.SetMin((points[0] * polyOffsetFactor) + offset_);
      bounds.SetMax((points[0] * polyOffsetFactor) + offset_);
   }
   int vlast = static_cast<int>(vertices.size());
   for (int i = 0; i < 4; i++) {
      Point3f offsetPoint = (points[i] * polyOffsetFactor) + offset_;
      vertices.emplace_back(Vertex(offsetPoint, (float)boneIndex, normal, color));
      bounds.Grow(offsetPoint);
   }
   
   //  it's up to the caller to get the winding right!
   indices.push_back(vlast + 0);  // first triangle...
   indices.push_back(vlast + 1);
   indices.push_back(vlast + 2);

   indices.push_back(vlast + 0);  // second triangle...
   indices.push_back(vlast + 2);
   indices.push_back(vlast + 3);
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

   if (flip_) {
      normal *= -1;
   }

   Point2f p0 = ToFloat(rect.min);
   Point2f p2 = ToFloat(rect.max);

   // In a rect min < max for all coords in the point.  To get the winding
   // correct, we need to re-order points depending on the direction of the
   // normal
   bool reverse_winding = (pi.reduced_coord == 2) ? (pi.normal_dir < 0) : (pi.normal_dir > 0);
   if (flip_) {
      reverse_winding = !reverse_winding;
   }

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
   flip_(false)
{
}

bool Mesh::IsEmpty() const
{
   return vertices.size() == 0;
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

void Mesh::ScaleBy(float scale)
{
   for (uint i = 0; i < vertices.size(); i++)
   {
      vertices[i].location[0] *= scale;
      vertices[i].location[1] *= scale;
      vertices[i].location[2] *= scale;
   }
}


Mesh& Mesh::FlipFaces()
{
   flip_ = !flip_;
   return *this;
}

Mesh& Mesh::AddVertices(Mesh const& other)
{   
   int offset = static_cast<int>(vertices.size());

   if (offset == 0) {
      vertices = other.vertices;
      indices = other.indices;
      bounds = other.bounds;
   } else {
      vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
      for (int32 i : other.indices) {
         indices.push_back(i + offset);
      }
      bounds.Grow(other.bounds);
   }
   return *this;
}

Mesh& Mesh::Clear()
{
   vertices.clear();
   indices.clear();
   bounds = Cube3f::zero;
   return *this;
}

void csg::RegionToMesh(csg::Region3 const& region, Mesh &mesh, csg::Point3f const& offset, bool optimizePlanes, uint32 boneIndex)
{
   mesh.SetOffset(offset);
   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      if (optimizePlanes) {
         csg::Region2 optPlane = plane;
         optPlane.OptimizeByMerge("converting region to mesh plane");
         mesh.AddRegion(optPlane, pi, boneIndex);
      } else {
         mesh.AddRegion(plane, pi, boneIndex);
      }
   });
}

template void Mesh::AddRegion(Region<double, 2> const& region, PlaneInfo<double, 3> const& p, uint32 boneIndex);
template void Mesh::AddRegion(Region<int, 2> const& region, PlaneInfo<int, 3> const& p, uint32 boneIndex);
template void Mesh::AddRect(Cube<double, 2> const& region, PlaneInfo<double, 3> const& p, uint32 boneIndex);
template void Mesh::AddRect(Cube<int, 2> const& region, PlaneInfo<int, 3> const& p, uint32 boneIndex);
