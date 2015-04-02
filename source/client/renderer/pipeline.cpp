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
#include "geometry_info.h"
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
void h3dRemoveNodeChecked(H3DNode node)
{
   if (node != 0) {
      h3dRemoveNode(node);
   }
}

static void EnablePolygonOffset(H3DNode node)
{
   h3dSetNodeParamI(node, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(node, H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(node, H3DModel::PolygonOffsetF, 1, -.01f);
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

   int vertexOffsets[2] = {0, (int)mesh.vertices.size()};
   int indexOffsets[2] = {0, (int)mesh.indices.size()};
   return h3dutCreateVoxelGeometryRes(geoName.c_str(), (VoxelGeometryVertex*)mesh.vertices.data(), vertexOffsets, (uint*)mesh.indices.data(), indexOffsets, 1);
}

void Pipeline::AddDesignationStripes(csg::Mesh& m, csg::Region2f const& panels)
{   
   float y = 0;
   for (csg::Rect2f const& cube: csg::EachCube(panels)) {      
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

void Pipeline::AddDesignationBorder(csg::Mesh& m, csg::EdgeMap2f& edgemap)
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

      csg::Point2f min = edge.min->location;
      csg::Point2f max = edge.max->location;
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
                                csg::Region2f const& plane,
                                csg::Color4 const& outline_color,
                                csg::Color4 const& stripes_color,
                                int useCoarseCollisionBox)
{
   csg::Mesh outline_mesh;
   csg::Mesh stripes_mesh;

   // flip the normal, since this is the bottom face
   outline_mesh.SetColor(outline_color);
   stripes_mesh.SetColor(stripes_color);

   csg::EdgeMap2f edgemap = csg::RegionTools2f().GetEdgeMap(plane);
   AddDesignationBorder(outline_mesh, edgemap);
   AddDesignationStripes(stripes_mesh, plane);

   RenderNodePtr group = RenderNode::CreateGroupNode(parent, "designation group node");
   RenderNodePtr stripes = CreateRenderNodeFromMesh(group->GetNode(), stripes_mesh)
      ->SetMaterial("materials/designation/stripes.material.json");

   h3dSetNodeParamI(stripes->GetNode(), H3DModel::UseCoarseCollisionBoxI, useCoarseCollisionBox);
   EnablePolygonOffset(stripes->GetNode());

   RenderNodePtr outline = CreateRenderNodeFromMesh(group->GetNode(), outline_mesh)
      ->SetMaterial("materials/designation/outline.material.json");

   h3dSetNodeParamI(outline->GetNode(), H3DModel::UseCoarseCollisionBoxI, useCoarseCollisionBox);
   EnablePolygonOffset(outline->GetNode());

   group->AddChild(stripes);
   group->AddChild(outline);
   return group;
}

RenderNodePtr
Pipeline::CreateStockpileNode(H3DNode parent,
                              csg::Region2f const& plane,
                              csg::Color4 const& interior_color,
                              csg::Color4 const& border_color)
{
   return CreateXZBoxNode(parent, plane, interior_color, border_color, 1.0);
}

RenderNodePtr
Pipeline::CreateSelectionNode(H3DNode parent,
                              csg::Region2f const& plane,
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
                                  csg::Region3f const& region,
                                  csg::Color4 const& edge_color,
                                  csg::Color4 const& face_color,
                                  std::string const& material)
{
   csg::Point3f offset(-0.5, 0, -0.5); // offset for terrain alignment
   csg::RegionTools3f tools;

   RenderNodePtr group_node = RenderNode::CreateGroupNode(parent, "RegionOutlineNode");

   if (edge_color.a != 0) {
      H3DNode edge_node = h3dRadiantAddDebugShapes(group_node->GetNode(), "edge_node");

      tools.ForEachUniqueEdge(region, [&edge_node, &offset, &edge_color](csg::EdgeInfo3f const& edge_info) {
         h3dRadiantAddDebugLine(edge_node, edge_info.min + offset, edge_info.max + offset, edge_color);
      });

      h3dRadiantCommitDebugShape(edge_node);
   }

   if (face_color.a != 0) {
      int tag = face_color.ToInteger();
      csg::Mesh mesh;

      tools.ForEachPlane(region, [&mesh, tag](csg::Region2f const& region2, csg::PlaneInfo3f const& plane_info) {
         for (csg::Rect2f const& rect : csg::EachCube(region2)) {
            csg::Rect2f face(rect);
            face.SetTag(tag);
            mesh.AddRect(face, plane_info);
         }
      });

      RenderNodePtr face_node = CreateRenderNodeFromMesh(group_node->GetNode(), mesh)
         ->SetMaterial(material);

      h3dSetNodeParamI(face_node->GetNode(), H3DModel::UseCoarseCollisionBoxI, 1);
      EnablePolygonOffset(face_node->GetNode());

      group_node->AddChild(face_node);
   }

   return group_node;
}

RenderNodePtr
Pipeline::CreateXZBoxNode(H3DNode parent,
                          csg::Region2f const& plane,
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

   RenderNodePtr interior = CreateRenderNodeFromMesh(group->GetNode(), mesh)
      ->SetMaterial("materials/transparent.material.json");

   h3dSetNodeParamI(interior->GetNode(), H3DModel::UseCoarseCollisionBoxI, 1);
   EnablePolygonOffset(interior->GetNode());

   group->AddChild(interior);
   return group;
}

void Pipeline::CreateXZBoxNodeGeometry(csg::Mesh& mesh, 
                                       csg::Region2f const& region, 
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

   
   csg::Rect2f bounds = region.GetBounds();
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

H3DNode Pipeline::CreateVoxelMeshNode(H3DNode parent, GeometryInfo const& geo)
{
   SharedMaterial material = GetSharedMaterial("materials/voxel.material.json");
   return CreateVoxelMeshNode(parent, geo, material);
}

H3DNode Pipeline::CreateVoxelMeshNode(H3DNode parent, GeometryInfo const& geo, SharedMaterial material)
{
   std::string meshName = BUILD_STRING("mesh" << unique_id_++);

   H3DNode meshNode = h3dAddVoxelMeshNode(parent, meshName.c_str(), material.get(), geo.geo.get());
   if (geo.noInstancing) {
      h3dSetNodeParamI(meshNode, H3DVoxelMeshNodeParams::NoInstancingI, true);
   }
   return meshNode;
}

void Pipeline::CreateSharedGeometryFromGenerator(MaterialToGeometryMapPtr& geometryPtr, ResourceCacheKey const& key, csg::ColorToMaterialMap const& colormap, CreateMeshLodLevelFn const& create_mesh_fn, bool noInstancing)
{
   // The meshes which get generated are determined greatly by the colormap.  For example, we might have
   // a colormap which remaps every color to red, which of course would generate different meshes than
   // one with an empty colormap.  Because of this, we can't cache the individual pieces of geometry we
   // generate during this process.  We have to cache the whole thing!
   ResourceCacheKey cacheKey(key);

   // Genearte a cache key.  csg::ColorToMaterialMap is sorted, so we know we'll generate the
   // same key every time.
   for (auto const& entry : colormap) {
      csg::MaterialName material = entry.second;
      cacheKey.AddElement("color", entry.first);
      cacheKey.AddElement("material", entry.second);
   }

   auto i = _materialToGeometryCache.find(cacheKey);
   if (i != _materialToGeometryCache.end()) {
      geometryPtr = i->second;
      return;
   }

   // Create all the geometry and cache it
   geometryPtr = std::make_shared<MaterialToGeometryMap>();
   MaterialToGeometryMap& geometry = *geometryPtr;

   // Accumulate the vertex and index data for all meshes at all LOD levels
   // into `meshes`, remembering offets into buffers as we go
   csg::MaterialToMeshMap meshes;
   for (int i = 0; i < GeometryInfo::MAX_LOD_LEVELS; i++) {
      create_mesh_fn(meshes, colormap, i);

      for (auto const& entry : meshes) {
         csg::MaterialName material = entry.first;
         csg::Mesh const& mesh = entry.second;

         GeometryInfo& geo = geometry[material];
         geo.levelCount = GeometryInfo::MAX_LOD_LEVELS;
         geo.noInstancing = noInstancing;
         geo.vertexIndices[i + 1] = (int)mesh.vertices.size();
         geo.indexIndicies[i + 1] = (int)mesh.indices.size();
      }
   }

   // Create the actual vertex and index buffers
   for (auto& entry : geometry) {
      csg::MaterialName material = entry.first;
      GeometryInfo& geo = entry.second;

      csg::Mesh const& mesh = meshes[material];
      ConvertVoxelDataToGeometry((VoxelGeometryVertex *)mesh.vertices.data(), (uint *)mesh.indices.data(), geo);
   }

   // Finally, cache it!
   _materialToGeometryCache[key] = geometryPtr;
}

void Pipeline::CreateSharedGeometryFromMesh(GeometryInfo& geo, ResourceCacheKey const& key, csg::Mesh const& m, bool noInstancing)
{
   if (!GetSharedGeometry(key, geo)) {
      CreateGeometryFromMesh(geo, m);
      SetSharedGeometry(key, geo);
   }
}

void Pipeline::CreateGeometryFromMesh(GeometryInfo& geo, csg::Mesh const& m)
{
   geo.vertexIndices[1] = (int)m.vertices.size();
   geo.indexIndicies[1] = (int)m.indices.size();
   geo.levelCount = 1;
   geo.noInstancing = true;

   ConvertVoxelDataToGeometry((VoxelGeometryVertex *)m.vertices.data(), (uint *)m.indices.data(), geo);
}

void Pipeline::CreateSharedGeometryFromOBJ(GeometryInfo& geo, ResourceCacheKey const& key, std::istream& is, bool noInstancing)
{
   if (!GetSharedGeometry(key, geo)) {
      ConvertObjFileToGeometry(is, geo);
      geo.noInstancing = noInstancing;
      SetSharedGeometry(key, geo);
   }
}

void Pipeline::ConvertVoxelDataToGeometry(VoxelGeometryVertex *vertices, uint *indices, GeometryInfo& geo)
{
   std::string geoName = BUILD_STRING("geo" << unique_id_++);

   geo.type = GeometryInfo::VOXEL_GEOMETRY;
   geo.geo = h3dutCreateVoxelGeometryRes(geoName.c_str(),
                                         vertices,
                                         geo.vertexIndices,
                                         indices,
                                         geo.indexIndicies,
                                         geo.levelCount);
}

template <typename X, typename Y>
struct PairHash {
public:
   size_t operator()(std::pair<X, Y> const& x) const {
      return std::hash<X>()(x.first) ^ std::hash<Y>()(x.second);
   }
};

void Pipeline::ConvertObjFileToGeometry(std::istream& stream, GeometryInfo &geo)
{

   struct Point { float x, y, z; };
   Point pt;
   std::vector<Point> obj_verts;
   std::vector<Point> obj_norms;
   std::unordered_map<std::pair<int, int>, int, PairHash<int, int>> vertex_map;
   std::vector<float> vertices;
   std::vector<unsigned int> indices;
   std::vector<short> normalData;

   std::string line;

   auto packNormal = [](float coord) -> short {
      return static_cast<short>(coord * 32767);
   };

   auto addIndex = [&](int vi, int ni) {
      --vi; --ni;    // covert 1 based indices to 0 based.

      std::pair<int, int> key(vi, ni);
      auto i = vertex_map.find(key);
      if (i != vertex_map.end()) {
         indices.push_back(i->second);
      } else {
         // create a new point
         Point const& v = obj_verts[vi];
         Point const& n = obj_norms[ni];

         vertices.push_back(v.x);
         vertices.push_back(v.y);
         vertices.push_back(v.z);

         normalData.push_back(packNormal(n.x));
         normalData.push_back(packNormal(n.y));
         normalData.push_back(packNormal(n.z));

         int index = (int)vertex_map.size();
         vertex_map[key] = index;
         indices.push_back(index);
      }
   };
   while (stream.good()) {
      std::vector<std::string> tokens;

      std::getline(stream, line);

      if (line.size() > 1) {
         char t0 = line[0], t1 = line[1];
         if (t0 == 'v') {
            if (sscanf(line.c_str(), "%*s %f %f %f", &pt.x, &pt.y, &pt.z) == 3) {
               if (t1 == 'n') {
                  obj_norms.push_back(pt);
               } else if (::isspace(t1)) {
                  obj_verts.push_back(pt);
               }
            }
         } else if (t0 == 'f') {
            int v[4], n[4]; 
            // textured triangle.  ignore the texture.
            if (sscanf(line.c_str(), "%*s %d/%*d/%d %d/%*d/%d %d/%*d/%d", &v[0], &n[0], &v[1], &n[1], &v[2], &n[2]) == 6) {
               addIndex(v[0], n[0]);
               addIndex(v[1], n[1]);
               addIndex(v[2], n[2]);
            }
            // textured quad.  ignore the texture.  convert to two triangles
            int c;
            if ((c = sscanf(line.c_str(), "%*s %d/%*d/%d %d/%*d/%d %d/%*d/%d %d/%*d/%d", &v[0], &n[0], &v[1], &n[1], &v[2], &n[2], &v[3], &n[3])) == 8) {
               addIndex(v[0], n[0]);
               addIndex(v[1], n[1]);
               addIndex(v[2], n[2]);

               addIndex(v[0], n[0]);
               addIndex(v[2], n[2]);
               addIndex(v[3], n[3]);
            }
         }
      }
   }
   
   std::string geoName = BUILD_STRING("geo" << unique_id_++);
   geo.vertexIndices[1] = (int)vertices.size() / 3;
   geo.indexIndicies[1] = (int)indices.size();
   geo.levelCount = 1;
   geo.type = GeometryInfo::HORDE_GEOMETRY;
   geo.geo = h3dutCreateGeometryRes(geoName.c_str(), (int)vertices.size() / 3, (int)indices.size(), vertices.data(),
                                    indices.data(), normalData.data(), nullptr, nullptr, nullptr, nullptr);
}

RenderNodePtr Pipeline::CreateRenderNodeFromMesh(H3DNode parent, csg::Mesh const& m)
{
   GeometryInfo geo;
   Pipeline::GetInstance().CreateGeometryFromMesh(geo, m);
   return RenderNode::CreateVoxelModelNode(parent, geo);
}