#ifndef _RADIANT_CSG_MESHTOOLS_H
#define _RADIANT_CSG_MESHTOOLS_H

#include <vector>
#include "util.h"
#include "color.h"
#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

typedef std::unordered_map<int, Color4> TagToColorMap;

class mesh_tools
{
public:
   // xxx: must match definition of VoxelGeometryVertex, exactly
   struct vertex {
      Point3f   location; // must be at offset 0
      Point3f   normal;
      Point4f   color;
      vertex(Point3f const& p, Point3f const& n, Point4f const& c) : location(p), normal(n), color(c) { }
      vertex(Point3f const& p, Point3f const& n, Color3 const& c) : location(p), normal(n), color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f) { }
      vertex(Point3f const& p, Point3f const& n, Color4 const& c) : location(p), normal(n), color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f) { }
   };

   struct mesh {
      std::vector<vertex>  vertices;
      std::vector<int32>   indices;
      Cube3f               bounds;

      mesh();

      mesh& SetOffset(csg::Point3f const& offset);
      mesh& SetColor(csg::Color4 const& color);
      mesh& FlipFaces();
      // This is the one, true add face.  move over to it...
      void AddFace(Point3f const points[], Point3f const& normal, Color4 const& color);
      template <class S> void AddRegion(Region<S, 2> const& region, PlaneInfo<S, 3> const& pi);
      template <class S> void AddRect(Cube<S, 2> const& region, PlaneInfo<S, 3> const& pi);

      // xxx: nuke this one!
      void add_face(Point3f const points[], Point3f const& normal, Point4f const& color);
   private:
      friend mesh_tools;
      csg::Color4     color_;
      bool            override_color_;
      bool            flip_;
      Point3f         offset_;
   };

public:
   mesh_tools();

   mesh_tools& SetOffset(Point3f const& offset);
   mesh_tools& SetColorMap(TagToColorMap const& colorMap);
   mesh ConvertRegionToMesh(Region3 const& region); // xxx: GET RID OF THIS BEFORE CHEKGIN IN!!

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

   struct PlaneInfoX {
      int      i;
      int      x;
      int      y;
      int      plane_value;
      int      normal_dir;
   };
   typedef std::function<void(Region2 const& region, PlaneInfoX const& info)> ForEachRegionPlaneCb;

   struct EdgeInfo {
      csg::Point3    min;
      csg::Point3    max;
      csg::Point3    normal;
   };
   typedef std::function<void(EdgeInfo const& info)> ForEachRegionEdgeCb;

   void ForEachRegionPlane(Region3 const& region, int flags, ForEachRegionPlaneCb const& cb);
   void ForEachRegionSegment(Region2 const& region, int flags, ForEachRegionSegmentCb const& cb);
   void ForEachRegionEdge(Region3 const& region, int flags, ForEachRegionEdgeCb const& cb);
private:
   typedef std::unordered_map<int, Region1> SegmentMap;
   typedef std::unordered_map<int, Region2> PlaneMap;
   typedef std::unordered_map<int, EdgeList> EdgeListMap;

   void ForEachRegionSegment(SegmentMap const& front, SegmentMap const& back, SegmentInfo pi, int normal_dir, int flags, ForEachRegionSegmentCb const& cb);
   void ForEachRegionPlane(PlaneMap const& front, PlaneMap const& back, PlaneInfoX pi, int normal_dir, int flags, ForEachRegionPlaneCb const& cb);

private:
   void AddRegionToMesh(Region2 const& region, PlaneInfoX const& p, mesh& m);
   void add_face(Point3f const points[], Point3f normal, mesh& m);

private:
   TagToColorMap const* colorMap_;
   Point3f              offset_;
};

void RegionToMesh(csg::Region3 const& region, mesh_tools::mesh &mesh, csg::Point3f const& offset, bool optimizePlanes);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_MESHTOOLS_H
