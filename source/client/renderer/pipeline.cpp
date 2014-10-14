#include "pch.h"
#include <regex>
#include "radiant_macros.h"
#include "pipeline.h"
#include "metrics.h"
#include "Horde3DUtils.h"
#include "horde3d/Source/Horde3DEngine/egVoxelGeometry.h"
#include "renderer.h"
#include "lib/voxel/qubicle_file.h"
#include "resources/res_manager.h"
#include "csg/iterators.h"
#include "csg/region_tools.h"
#include <fstream>
#include <unordered_set>

#define P_LOG(level)   LOG(renderer.pipeline, level)

namespace metrics = ::radiant::metrics;
using ::radiant::csg::Point3;

using namespace ::radiant;
using namespace ::radiant::client;

DEFINE_SINGLETON(Pipeline);

static const struct {
   int i, u, v, mask;
   csg::Point3f normal;
} qubicle_node_edges__[] = {
   { 2, 0, 1, voxel::FRONT_MASK,  csg::Point3f( 0,  0,  1) },
   { 2, 0, 1, voxel::BACK_MASK,   csg::Point3f( 0,  0, -1) },
   { 1, 0, 2, voxel::TOP_MASK,    csg::Point3f( 0,  1,  0) },
   { 1, 0, 2, voxel::BOTTOM_MASK, csg::Point3f( 0, -1,  0) },
   { 0, 2, 1, voxel::RIGHT_MASK,  csg::Point3f(-1,  0,  0) },
   { 0, 2, 1, voxel::LEFT_MASK,   csg::Point3f( 1,  0,  0) },
};

/*
 * -- h3dUnloadResourceNop
 *
 * Used to "remove" resources from the SharedMaterial type.  Since materials are
 * highly reused, we don't bother unloading them.  Someone is almost certainly
 * going to come load it again!
 */
void h3dUnloadResourceNop(H3DRes)
{
}

/*
 * -- h3dRemoveResourceChecked
 *
 * Check for 0 before calling into horde, since it gets really mad if we don't
 * (spewing the log).
 */
void h3dRemoveResourceChecked(H3DRes res)
{
   if (res != 0) {
      h3dRemoveResource(res);
   }
}

/*
 * -- h3dRemoveNodeChecked
 *
 * Check for 0 before calling into horde, since it gets really mad if we don't
 * (spewing the log).
 */
void h3dRemoveNodeChecked(H3DRes node)
{
   if (node != 0) {
      h3dRemoveNode(node);
   }
}

Pipeline::Pipeline() :
   unique_id_(1)
{
}

Pipeline::~Pipeline()
{
}

H3DRes Pipeline::CreateVoxelGeometryFromRegion(std::string const& geoName, csg::Region3 const& region)
{
   csg::Mesh mesh;
   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      mesh.AddRegion(plane, pi);
   });

   int vertexOffsets[2] = {0, mesh.vertices.size()};
   int indexOffsets[2] = {0, mesh.indices.size()};
   return h3dutCreateVoxelGeometryRes(geoName.c_str(), (VoxelGeometryVertex*)mesh.vertices.data(), vertexOffsets, (uint*)mesh.indices.data(), indexOffsets, 1);
}

void Pipeline::AddDesignationStripes(csg::Mesh& m, csg::Region2 const& panels)
{   
   float y = 0;
   for (csg::Rect2 const& c: csg::EachCube(panels)) {      
      csg::Rect2f cube = ToFloat(c);
      csg::Point2f size = cube.GetSize();
      for (double i = 0; i < size.y; i ++) {
         // xxx: why do we have to use a clockwise winding here?
         double x1 = std::min(i + 1.0, size.x);
         double x2 = std::min(i + 0.5, size.x);
         double y1 = std::max(i + 1.0 - size.x, 0.0);
         double y2 = std::max(i + 0.5 - size.x, 0.0);
         csg::Point3f points[] = {
            csg::Point3f(cube.min.x, y, cube.min.y + i + 1.0),
            csg::Point3f(cube.min.x + x1, y, cube.min.y + y1),
            csg::Point3f(cube.min.x + x2, y, cube.min.y + y2),
            csg::Point3f(cube.min.x, y, cube.min.y + i + 0.5),
         };
         m.AddFace(points, csg::Point3f::unitY);
      }
      for (double i = 0; i < size.x; i ++) {
         // xxx: why do we have to use a clockwise winding here?
         double x0 = i + 0.5;
         double x1 = i + 1.0;
         double x2 = std::min(x1 + size.y, size.x);
         double x3 = std::min(x0 + size.y, size.x);
         double y2 = -std::min(x2 - x1, size.y);
         double y3 = -std::min(x3 - x0, size.y);
         csg::Point3f points[] = {
            csg::Point3f(cube.min.x + x0, y, cube.max.y),
            csg::Point3f(cube.min.x + x1, y, cube.max.y),
            csg::Point3f(cube.min.x + x2, y, cube.max.y + y2),
            csg::Point3f(cube.min.x + x3, y, cube.max.y + y3),
         };
         m.AddFace(points, csg::Point3f::unitY);
      }
   }
}

void Pipeline::AddDesignationBorder(csg::Mesh& m, csg::EdgeMap2& edgemap)
{
   float thickness = 0.25f;
   csg::PlaneInfo3f pi;
   pi.reduced_coord = 1;
   pi.reduced_value = 0;
   pi.x = 0;
   pi.y = 2;
   pi.normal_dir = 1;

   for (auto const& edge : edgemap) {
      // Compute the iteration direction
      int t = 0, n = 1;
      if (edge.normal.x) {
         t = 1, n = 0;
      }

      csg::Point2f min = ToFloat(edge.min->location);
      csg::Point2f max = ToFloat(edge.max->location);
      max -= ToFloat(edge.normal) * thickness;

      csg::Rect2f dash = csg::Rect2f::Construct(min, max);
      double min_t = dash.min[t];
      double max_t = dash.max[t];

      // Min corner...
      if (edge.min->accumulated_normals.Length() > 1) {
         dash.min[t] = min_t;
         dash.max[t] = dash.min[t] + 0.75f;
         m.AddRect(dash, ToFloat(pi));
         min_t += 1.0;
      }
      // Max corner...
      if (edge.max->accumulated_normals.Length() > 1) {
         dash.max[t] = max_t;
         dash.min[t] = dash.max[t] - 0.75;
         m.AddRect(dash, ToFloat(pi));
         max_t -= 1.0;
      }
      // Range...
      for (double v = min_t; v < max_t; v++) {
         dash.min[t] = v + 0.25;
         dash.max[t] = v + 0.75;
         m.AddRect(dash, ToFloat(pi));
      }
   }
}

RenderNodePtr
Pipeline::CreateDesignationNode(H3DNode parent,
                                csg::Region2 const& plane,
                                csg::Color4 const& outline_color,
                                csg::Color4 const& stripes_color)
{
   csg::Mesh outline_mesh;
   csg::Mesh stripes_mesh;

   // flip the normal, since this is the bottom face
   outline_mesh.SetColor(outline_color);
   stripes_mesh.SetColor(stripes_color);

   csg::EdgeMap2 edgemap = csg::RegionTools2().GetEdgeMap(plane);
   AddDesignationBorder(outline_mesh, edgemap);
   AddDesignationStripes(stripes_mesh, plane);

   RenderNodePtr group = RenderNode::CreateGroupNode(parent, "designation group node");
   RenderNodePtr stripes = RenderNode::CreateCsgMeshNode(group->GetNode(), stripes_mesh)
      ->SetMaterial("materials/designation/stripes.material.xml");

   h3dSetNodeParamI(stripes->GetNode(), H3DModel::UseCoarseCollisionBoxI, 1);
   h3dSetNodeParamI(stripes->GetNode(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(stripes->GetNode(), H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(stripes->GetNode(), H3DModel::PolygonOffsetF, 1, -.01f);

   RenderNodePtr outline = RenderNode::CreateCsgMeshNode(group->GetNode(), outline_mesh)
      ->SetMaterial("materials/designation/outline.material.xml");

   h3dSetNodeParamI(outline->GetNode(), H3DModel::UseCoarseCollisionBoxI, 1);
   h3dSetNodeParamI(outline->GetNode(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(outline->GetNode(), H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(outline->GetNode(), H3DModel::PolygonOffsetF, 1, -.01f);  

   group->AddChild(stripes);
   group->AddChild(outline);
   return group;
}

RenderNodePtr
Pipeline::CreateStockpileNode(H3DNode parent,
                                csg::Region2 const& plane,
                                csg::Color4 const& interior_color,
                                csg::Color4 const& border_color)
{
   return CreateXZBoxNode(parent, plane, interior_color, border_color, 1.0);
}

RenderNodePtr
Pipeline::CreateSelectionNode(H3DNode parent,
                              csg::Region2 const& plane,
                              csg::Color4 const& interior_color,
                              csg::Color4 const& border_color)
{
   return CreateXZBoxNode(parent, plane, interior_color, border_color, 0.2f);
}

typedef std::pair<csg::Point3, csg::Point3> LineSegment;

// template specialization for the hash of a LineSegment
namespace std {
   template <>
   class std::hash<LineSegment>
   {
   public:
      size_t operator()(LineSegment const& segment) const {
         // use boost::hash_combine if you want something better
         return 51 * csg::Point3::Hash()(segment.first) + csg::Point3::Hash()(segment.second);
      }
   };
}

RenderNodePtr
Pipeline::CreateRegionOutlineNode(H3DNode parent,
                                  csg::Region3 const& region,
                                  csg::Color4 const& color)
{
   csg::Point3f offset(-0.5, 0, -0.5); // offset for terrain alignment
   H3DNode node = h3dRadiantAddDebugShapes(parent, "RegionOutlineNode");
   csg::RegionTools3 tools;
   std::unordered_set<LineSegment> edges;

   tools.ForEachEdge(region, [&edges, &node, &offset, &color](csg::EdgeInfo3 const& edge_info) {
      LineSegment edge(edge_info.min, edge_info.max);
      auto iterator = edges.find(edge);
      if (iterator != edges.end()) {
         // discard edges with the same endpoints (we don't care that their normals are different)
         return;
      }

      edges.insert(edge);
      h3dRadiantAddDebugLine(node, csg::ToFloat(edge.first) + offset, csg::ToFloat(edge.second) + offset, color);
   });

   h3dRadiantCommitDebugShape(node);

   return std::make_shared<RenderNode>(node);
}

RenderNodePtr
Pipeline::CreateXZBoxNode(H3DNode parent,
                          csg::Region2 const& plane,
                          csg::Color4 const& interior_color,
                          csg::Color4 const& border_color,
                          float border_size)
{
   csg::RegionTools2 tools;

   csg::Mesh mesh;

   //interior_mesh.SetColor(interior_color);
   //border_mesh.SetColor(border_color);

   CreateXZBoxNodeGeometry(mesh, plane, interior_color, border_color, border_size);

   RenderNodePtr group = RenderNode::CreateGroupNode(parent, "designation group node");

   RenderNodePtr interior = RenderNode::CreateCsgMeshNode(group->GetNode(), mesh)
      ->SetMaterial("materials/transparent.material.xml");

   h3dSetNodeParamI(interior->GetNode(), H3DModel::UseCoarseCollisionBoxI, 1);
   h3dSetNodeParamI(interior->GetNode(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(interior->GetNode(), H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(interior->GetNode(), H3DModel::PolygonOffsetF, 1, -.01f);

   group->AddChild(interior);
   return group;
}

void Pipeline::CreateXZBoxNodeGeometry(csg::Mesh& mesh, 
                                       csg::Region2 const& region, 
                                       csg::Color4 const& interior_color, 
                                       csg::Color4 const& border_color, 
                                       float border_size)
{
   csg::PlaneInfo3f pi;
   pi.reduced_coord = 1;
   pi.reduced_value = 0;
   pi.x = 0;
   pi.y = 2;
   pi.normal_dir = 1;

   
   csg::Rect2f bounds = csg::ToFloat(region.GetBounds());
   bounds.SetTag(border_color.ToInteger());

   csg::Region2f boxRegion;
   boxRegion.Add(bounds);

   // Add a border if there's room (width/height of the node is > twice the border size)
   if (bounds.min.x < bounds.max.x - (border_size * 2) && bounds.min.y < bounds.max.y - (border_size * 2)) {
      csg::Point2f min = csg::ToFloat(bounds.min) + csg::Point2f(border_size, border_size);
      csg::Point2f max = csg::ToFloat(bounds.max) - csg::Point2f(border_size, border_size);
      boxRegion.Add(csg::Rect2f(min, max, interior_color.ToInteger()));
   }
   mesh.AddRegion(boxRegion, pi);
}

SharedMaterial Pipeline::GetSharedMaterial(std::string const& material)
{
   // xxx: share these one day?
   return h3dAddResource(H3DResTypes::Material, material.c_str(), 0);
}

bool Pipeline::GetSharedGeometry(ResourceCacheKey const& key, GeometryInfo& geo)
{
   auto i = geometry_cache_.find(key);
   if (i == geometry_cache_.end()) {
      return false;
   }
   geo = i->second;
   return true;
}

void Pipeline::SetSharedGeometry(ResourceCacheKey const& key, GeometryInfo const& geo)
{
   ASSERT(geometry_cache_.find(key) == geometry_cache_.end());
   geometry_cache_[key] = geo;
}
