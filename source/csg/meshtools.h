#ifndef _RADIANT_CSG_MESHTOOLS_H
#define _RADIANT_CSG_MESHTOOLS_H

#include <vector>
#include "util.h"
#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

class mesh_tools
{
public:
   // xxx: must match definition of VoxelGeometryVertex, exactly
   struct vertex {
      Point3f   location; // must be at offset 0
      Point3f   normal;
      Point3f   color;
      vertex(Point3f const& p, Point3f const& n, Point3f const& c) : location(p), normal(n), color(c) { }
   };

   struct mesh {
      std::vector<vertex>  vertices;
      std::vector<int32>   indices;
      Cube3f               bounds;

      // This is the one, true add face.  move over to it... (and when you're done, make color a Color4)
      void AddFace(Point3f const points[], Point3f const& normal, Color3 const& color) {
         add_face(points, normal, Point3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f), false);
      }
      void add_face(Point3f const points[], Point3f const& normal, Point3f const& color, bool rewind_based_on_normal = true);
   private:
      friend mesh_tools;
      Point3f         offset_;
   };

   typedef std::function<void(int, Point3f const [], Point3f const&, mesh_tools::mesh&)> tesselator_fn;
   typedef std::unordered_map<int, tesselator_fn> tesselator_map;

public:
   mesh_tools();

   mesh_tools& SetOffset(Point3f const& offset);
   mesh_tools& SetTesselator(tesselator_map const& t);
   mesh ConvertRegionToMesh(Region3 const& region);
   mesh ConvertRegionToOutline(const Region3& r3, float thickness, csg::Color3 const& color);

   enum {
      INCLUDE_HIDDEN_FACES    = (1 << 1)
   };

   struct SegmentInfo {
      int      i;
      int      x;
      int      line_value;
      int      normal_dir;
   };
   typedef std::function<void(Region1 const& rect, SegmentInfo const& info)> ForEachRegionSegmentCb;

   struct PlaneInfo {
      int      i;
      int      x;
      int      y;
      int      plane_value;
      int      normal_dir;
   };
   typedef std::function<void(Region2 const& region, PlaneInfo const& info)> ForEachRegionPlaneCb;

   struct EdgeInfo {
      csg::Point3    min;
      csg::Point3    max;
      csg::Point3    normal;
   };
   typedef std::function<void(EdgeInfo const& info)> ForEachRegionEdgeCb;

   void ForEachRegionPlane(Region3 const& region, int flags, ForEachRegionPlaneCb cb);
   void ForEachRegionSegment(Region2 const& region, int flags, ForEachRegionSegmentCb cb);
   void ForEachRegionEdge(Region3 const& region, int flags, ForEachRegionEdgeCb cb);
private:
   typedef std::unordered_map<int, Region1> SegmentMap;
   typedef std::unordered_map<int, Region2> PlaneMap;
   typedef std::unordered_map<int, EdgeList> EdgeListMap;

   void ForEachRegionSegment(SegmentMap const& front, SegmentMap const& back, SegmentInfo pi, int normal_dir, int flags, ForEachRegionSegmentCb cb);
   void ForEachRegionPlane(PlaneMap const& front, PlaneMap const& back, PlaneInfo pi, int normal_dir, int flags, ForEachRegionPlaneCb cb);

private:
   void AddRegionToMesh(Region2 const& region, PlaneInfo const& p, mesh& m);
   void add_face(Point3f const points[], Point3f normal, mesh& m);

private:
   tesselator_map const*   tesselators_;
   Point3f                 offset_;
};

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_MESHTOOLS_H
