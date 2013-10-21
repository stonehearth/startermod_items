#include "pch.h"
#include "color.h"
#include "meshtools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

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

void mesh_tools::ForEachRegionPlane(PlaneMap const& front, PlaneMap const& back, PlaneInfo pi, int normal_dir, int flags, ForEachRegionPlaneCb cb)
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
   static const PlaneInfo plane_data [] = {
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

std::vector<Point3f> mesh_tools::ConvertRegionToOutline(const Region3& r3)
{
   std::vector<Point3f> lines;

   ForEachRegionPlane(r3, 0, [&](Region2 const& r2, PlaneInfo const& pi) {
      csg::Point3f min, max;
      min[pi.i] = (float)pi.plane_value;
      max[pi.i] = (float)pi.plane_value;
      ForEachRegionSegment(r2, 0, [&](Region1 const& r1, SegmentInfo const& info) {
         int x = info.i == 0 ? pi.y : pi.x;
         int y = info.i == 0 ? pi.x : pi.y;
         min[y] = (float)info.line_value;
         max[y] = (float)info.line_value;
         for (Line1 const& line : r1) {
            min[x] = (float)line.min.x;
            max[x] = (float)line.max.x;
            lines.push_back(min + offset_);
            lines.push_back(max + offset_);
         }
      });
   });
   return lines;
}

mesh_tools::mesh mesh_tools::ConvertRegionToMesh(const Region3& region)
{   
   mesh m;
   m.offset_ = offset_;

   ForEachRegionPlane(region, 0, [&](Region2 const& r2, PlaneInfo const& pi) {
      add_region(r2, pi, m);
   });
   return m;
}

void mesh_tools::add_region(Region2 const& region, PlaneInfo const& pi, mesh& m)
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
         Point3f color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
         m.add_face(points, normal, color);
      } else {
         auto i = tesselators_->find(tag);
         if (i != tesselators_->end()) {
            i->second(tag, points, normal, m);
         } else {
            m.add_face(points, normal, Point3f(0.5, 0.5, 0.5));
         }
      }
   }
}

void mesh_tools::mesh::add_face(Point3f const points[], Point3f const& normal, Point3f const& color)
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
