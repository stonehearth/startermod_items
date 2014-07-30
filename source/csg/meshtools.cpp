#include "pch.h"
#include "color.h"
#include "meshtools.h"
#include "region_tools.h"
#include "region_tools_traits.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define MT_LOG(level)      LOG(csg.meshtools, level)

mesh_tools::mesh_tools() :
   tesselators_(nullptr),
   offset_(-0.5f, 0.0f, -0.5f)
{
}

mesh_tools& mesh_tools::SetOffset(Point3f const& offset)
{
   offset_ = offset;
   return *this;
}

mesh_tools& mesh_tools::SetTesselator(tesselator_map const& t)
{
   tesselators_ = &t;
   return *this;
}

void mesh_tools::ForEachRegionSegment(SegmentMap const& front, SegmentMap const& back, SegmentInfo pi, int normal_dir, int flags, ForEachRegionSegmentCb cb)
{
   pi.normal_dir = normal_dir;

   for (const auto& entry : front) {
      pi.line_value = entry.first;
      Region1 const& region = entry.second;

      if ((flags & INCLUDE_HIDDEN_FACES) != 0) {
         cb(region, pi);
      } else {
         auto i = back.find(pi.line_value);
         if (i != back.end()) {
            cb(region - i->second, pi);
         } else {
            cb(region, pi);
         }
      }
   }
}

void mesh_tools::ForEachRegionSegment(Region2 const& region, int flags, ForEachRegionSegmentCb cb)
{
   // xxx: this is in no way thread safe! (see SH-8)
   static const SegmentInfo segment_data [] = {
      { 0, 1, 0xd3adb33f, 0xd3adb33f },
      { 1, 0, 0xd3adb33f, 0xd3adb33f },
   };
   for (auto const& sd : segment_data) {
      SegmentMap front, back;

      for (Rect2 const& r : region) {
         Line1 line(Point1(r.min[sd.x]), Point1(r.max[sd.x]), r.GetTag());
         front[r.min[sd.i]].AddUnique(line);
         back[r.max[sd.i]].AddUnique(line);
      }
      ForEachRegionSegment(front, back, sd, -1, flags, cb);
      ForEachRegionSegment(back, front, sd,  1, flags, cb);
   }
}

void mesh_tools::ForEachRegionPlane(PlaneMap const& front, PlaneMap const& back, PlaneInfoX pi, int normal_dir, int flags, ForEachRegionPlaneCb cb)
{
   pi.normal_dir = normal_dir;

   for (const auto& entry : front) {
      pi.plane_value = entry.first;
      Region2 const& region = entry.second;

      if ((flags & INCLUDE_HIDDEN_FACES) != 0) {
         cb(region, pi);
      } else {
         auto i = back.find(pi.plane_value);
         if (i != back.end()) {
            cb(region - i->second, pi);
         } else {
            cb(region, pi);
         }
      }
   }
}

// Iterate through every outward facing plane in the region...
void mesh_tools::ForEachRegionPlane(Region3 const& region, int flags, ForEachRegionPlaneCb cb)
{
   // xxx: this is in no way thread safe! (see SH-8)
   static const PlaneInfoX plane_data [] = {
      { 0, 2, 1, 0xd3adb33f, 0xd3adb33f },
      { 1, 0, 2, 0xd3adb33f, 0xd3adb33f },
      { 2, 0, 1, 0xd3adb33f, 0xd3adb33f },
   };
   for (auto const& pd : plane_data) {
      PlaneMap front, back;

      for (Cube3 const& c : region) {
         Rect2 rect(Point2(c.min[pd.x], c.min[pd.y]), Point2(c.max[pd.x], c.max[pd.y]), c.GetTag());
         front[c.min[pd.i]].AddUnique(rect);
         back[c.max[pd.i]].AddUnique(rect);
      }
      ForEachRegionPlane(front, back, pd, -1, flags, cb);
      ForEachRegionPlane(back, front, pd,  1, flags, cb);
   }
}

void mesh_tools::ForEachRegionEdge(Region3 const& region, int flags, ForEachRegionEdgeCb cb)
{
   EdgeInfo ei;
   ForEachRegionPlane(region, 0, [&](Region2 const& r2, PlaneInfoX const& pi) {
      ei.normal = Point3::zero;
      ei.min[pi.i] = pi.plane_value;
      ei.max[pi.i] = pi.plane_value;
      ei.normal[pi.i] = pi.normal_dir;
      ForEachRegionSegment(r2, 0, [&](Region1 const& r1, SegmentInfo const& si) {
         int x = si.i == 0 ? pi.y : pi.x;
         int y = si.i == 0 ? pi.x : pi.y;
         ei.min[y] = si.line_value;
         ei.max[y] = si.line_value;
         for (Line1 const& line : r1) {
            ei.min[x] = line.min.x;
            ei.max[x] = line.max.x;
            cb(ei);
         }
      });
   });
}

mesh_tools::mesh mesh_tools::ConvertRegionToMesh(const Region3& region)
{   
   mesh m;
   m.SetOffset(offset_);

   ForEachRegionPlane(region, 0, [&](Region2 const& r2, PlaneInfoX const& pi) {
      AddRegionToMesh(r2, pi, m);
   });
   return m;
}

void mesh_tools::AddRegionToMesh(Region2 const& region, PlaneInfoX const& pi, mesh& m)
{
   for (Rect2 const& r: region) {
      Point2 const& min = r.GetMin();
      Point2 const& max = r.GetMax();

      Point3f points[4];
      points[0][pi.x] = (float)min.x;
      points[0][pi.y] = (float)min.y;
      points[0][pi.i] = (float)pi.plane_value;

      points[1][pi.x] = (float)min.x;
      points[1][pi.y] = (float)max.y;
      points[1][pi.i] = (float)pi.plane_value;

      points[2][pi.x] = (float)max.x;
      points[2][pi.y] = (float)max.y;
      points[2][pi.i] = (float)pi.plane_value;

      points[3][pi.x] = (float)max.x;
      points[3][pi.y] = (float)min.y;
      points[3][pi.i] = (float)pi.plane_value;

      Point3f normal(0, 0, 0);
      normal[pi.i] = (float)pi.normal_dir;

      int tag = r.GetTag();

      // this is super gross.  instead of passing in a tess map, we should parameterize the 
      // mesh_tools object with a function that we can call...  or better yet, dump this
      // thing and do tesselation on the server!
      if (!tesselators_) {         
         Color3 c= Color3::FromInteger(tag);
         Point4f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
         m.add_face(points, normal, color);
      } else {
         auto i = tesselators_->find(tag);
         if (i != tesselators_->end()) {
            i->second(tag, points, normal, m);
         } else {
            // xxx: get the winding right...
            Color3 c = Color3::FromInteger(tag);
            m.add_face(points, normal, Point4f(c.r/255.0f, c.g/255.0f, c.b/255.0f, 1.0f));
         }
      }
   }
}

void mesh_tools::mesh::AddFace(Point3f const points[], Point3f const& normal, Color4 const& color)
{
   csg::Point4f c;
   if (override_color_) {
      c = Point4f(color_.r / 255.0f, color_.g / 255.0f, color_.b / 255.0f, color_.a / 255.0f);
   } else {
      c = Point4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
   }
   
   if (vertices.empty()) {
      bounds.SetMin(points[0] + offset_);
      bounds.SetMax(points[0] + offset_);
   }
   int vlast = vertices.size();
   for (int i = 0; i < 4; i++) {
      Point3f pt = points[i] + offset_;
      vertices.emplace_back(vertex(pt, normal, c));
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

void mesh_tools::mesh::add_face(Point3f const points[], Point3f const& normal, Point4f const& color)
{
   if (vertices.empty()) {
      bounds.SetMin(points[0] + offset_);
      bounds.SetMax(points[0] + offset_);
   }
   int vlast = vertices.size();
   for (int i = 0; i < 4; i++) {
      Point3f pt = points[i] + offset_;
      vertices.emplace_back(vertex(pt, normal, color));
      bounds.Grow(pt);
   }
   
   // xxx: this is a super gross hack.  the client should know what they mean
   // and we should do what they say!
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

template <typename S>
void mesh_tools::mesh::AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& p)
{
   for (Cube<S, 2> const& rect : region) {
      AddRect(rect, p);
   }
}

template <class S>
void mesh_tools::mesh::AddRect(Cube<S, 2> const& rect, PlaneInfo<S, 3> const& p)
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
   AddFace(points, normal, override_color_ ? color_ : Color4::FromInteger(rect.GetTag()));
}

mesh_tools::mesh::mesh() : 
   override_color_(false),
   offset_(-0.5f, 0.0f, -0.5f),
   flip_(false)
{
}

mesh_tools::mesh& mesh_tools::mesh::SetColor(csg::Color4 const& color)
{
   color_ = csg::Color4(color.r, color.g, color.b, color.a);
   override_color_ = true;
   return *this;
}

mesh_tools::mesh& mesh_tools::mesh::SetOffset(csg::Point3f const& offset)
{
   offset_ = offset;
   return *this;
}


mesh_tools::mesh& mesh_tools::mesh::FlipFaces()
{
   flip_ = !flip_;
   return *this;
}

template void mesh_tools::mesh::AddRegion(Region<float, 2> const& region, PlaneInfo<float, 3> const& p);
template void mesh_tools::mesh::AddRegion(Region<int, 2> const& region, PlaneInfo<int, 3> const& p);
template void mesh_tools::mesh::AddRect(Cube<float, 2> const& region, PlaneInfo<float, 3> const& p);
template void mesh_tools::mesh::AddRect(Cube<int, 2> const& region, PlaneInfo<int, 3> const& p);
