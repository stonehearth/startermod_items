#include "pch.h"
#include "color.h"
#include "meshtools.h"
#include "region_tools.h"
#include "region_tools_traits.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define MT_LOG(level)      LOG(csg.meshtools, level)

void Mesh::AddFace(Point3f const points[], Point3f const& normal)
{
   AddFace(points, normal, color_);
}

void Mesh::AddFace(Point3f const points[], Point3f const& normal, Color4 const& c)
{
   csg::Point4f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);

   if (vertices.empty()) {
      bounds.SetMin(points[0] + offset_);
      bounds.SetMax(points[0] + offset_);
   }
   int vlast = vertices.size();
   for (int i = 0; i < 4; i++) {
      Point3f pt = points[i] + offset_;
      vertices.emplace_back(Vertex(pt, normal, color));
      bounds.Grow(pt);
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
void Mesh::AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& p)
{
   for (Cube<S, 2> const& rect : region) {
      AddRect(rect, p);
   }
}

template <class S>
void Mesh::AddRect(Cube<S, 2> const& rect, PlaneInfo<S, 3> const& p)
{
   PlaneInfo<float, 3> pi = ToFloat(p);
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
   AddFace(points, normal, color);
}

Mesh::Mesh() : 
   override_color_(false),
   color_(0, 0, 0, 255),
   colorMap_(nullptr),
   offset_(-0.5f, 0.0f, -0.5f),
   flip_(false)
{
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


Mesh& Mesh::FlipFaces()
{
   flip_ = !flip_;
   return *this;
}

void csg::RegionToMesh(csg::Region3 const& region, Mesh &mesh, csg::Point3f const& offset, bool optimizePlanes)
{
   mesh.SetOffset(offset);
   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      if (optimizePlanes) {
         csg::Region2 optPlane = plane;
         optPlane.OptimizeByMerge();
         mesh.AddRegion(optPlane, pi);
      } else {
         mesh.AddRegion(plane, pi);
      }
   });
}

template void Mesh::AddRegion(Region<float, 2> const& region, PlaneInfo<float, 3> const& p);
template void Mesh::AddRegion(Region<int, 2> const& region, PlaneInfo<int, 3> const& p);
template void Mesh::AddRect(Cube<float, 2> const& region, PlaneInfo<float, 3> const& p);
template void Mesh::AddRect(Cube<int, 2> const& region, PlaneInfo<int, 3> const& p);
