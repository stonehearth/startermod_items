#include "pch.h"
#include "radiant_macros.h"
#include "pipeline.h"
#include "metrics.h"
#include "Horde3DUtils.h"
#include "horde3d/Source/Horde3DEngine/egVoxelGeometry.h"
#include "renderer.h"
#include "lib/voxel/qubicle_file.h"
#include "resources/res_manager.h"
#include "csg/region_tools.h"
#include <fstream>
#include <unordered_map>

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

Pipeline::Pipeline() :
   unique_id_(1)
{
}

Pipeline::~Pipeline()
{
}

H3DRes Pipeline::CreateVoxelGeometryFromRegion(const std::string& geoName, csg::Region3 const& region)
{
   auto& mesh = CreateMeshFromRegion(region);
   return h3dutCreateVoxelGeometryRes(geoName.c_str(), (VoxelGeometryVertex*)mesh.vertices.data(), mesh.vertices.size(), (uint*)mesh.indices.data(), mesh.indices.size());
}

H3DNode Pipeline::AddDynamicMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m, std::string const& material, int userFlags)
{   
   H3DRes geometry = ConvertMeshToGeometryResource(m);
   return CreateModelNode(parent, geometry, m.indices.size(), m.vertices.size(), material, userFlags);
}

H3DNode Pipeline::AddSharedMeshNode(H3DNode parent, ResourceCacheKey const& key, std::string const& material, std::function<void(csg::mesh_tools::mesh &)> create_mesh_fn)
{   
   H3DRes geometry;
   auto i = resource_cache_.find(key);
   if (i != resource_cache_.end()) {
      P_LOG(7) << "using cached geometry for " << key.GetDescription();
      geometry = i->second;
   } else {
      P_LOG(7) << "creating new geometry for " << key.GetDescription();
      csg::mesh_tools::mesh m;
      create_mesh_fn(m);
      geometry = ConvertMeshToGeometryResource(m);
      resource_cache_[key] = geometry;
   }

   int indexCount = h3dGetResParamI(geometry, Horde3D::VoxelGeometryResData::VoxelGeometryElem, 0, 
                                    Horde3D::VoxelGeometryResData::VoxelGeoIndexCountI);
   int vertexCount = h3dGetResParamI(geometry, Horde3D::VoxelGeometryResData::VoxelGeometryElem, 0, 
                                     Horde3D::VoxelGeometryResData::VoxelGeoVertexCountI);

   return CreateModelNode(parent, geometry, indexCount, vertexCount, material, UserFlags::None);
}


H3DRes Pipeline::ConvertMeshToGeometryResource(const csg::mesh_tools::mesh& m)
{
   std::string name = BUILD_STRING("geo" << unique_id_++);

   return h3dutCreateVoxelGeometryRes(name.c_str(),
      (VoxelGeometryVertex *)m.vertices.data(),
      m.vertices.size(),
      (uint *)m.indices.data(),
      m.indices.size());
}

csg::mesh_tools::mesh Pipeline::CreateMeshFromRegion(csg::Region3 const& region)
{
   csg::mesh_tools::mesh mesh;
   csg::RegionTools3().ForEachPlane(region, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      mesh.AddRegion(plane, pi);
   });

   return mesh;
}

H3DNode Pipeline::CreateModelNode(H3DNode parent, H3DRes geometry, int indexCount, int vertexCount, std::string const& material, int userFlags)
{
   ASSERT(!material.empty());

   std::string name = "mesh data ";

   std::string model_name = BUILD_STRING("model" << unique_id_++);
   std::string mesh_name = BUILD_STRING("mesh" << unique_id_++);

   H3DRes matRes = h3dAddResource(H3DResTypes::Material, material.c_str(), 0);
   H3DNode model_node = h3dAddVoxelModelNode(parent, model_name.c_str(), geometry);
   H3DNode mesh_node = h3dAddVoxelMeshNode(model_node, mesh_name.c_str(), matRes, 0, indexCount, 0, vertexCount - 1);
   h3dSetNodeParamI(mesh_node, H3DNodeParams::UserFlags, userFlags);

   // xxx: how do res, matRes, and mesh_node get deleted? - tony
   return model_node;
}

H3DNodeUnique Pipeline::CreateBlueprintNode(H3DNode parent,
                                            csg::Region3 const& model,
                                            float thickness,
                                            std::string const& material_path,
                                            csg::Point3f const& offset)
{
   H3DNode group = h3dAddGroupNode(parent, BUILD_STRING("blueprint node " << unique_id_++).c_str());

   csg::mesh_tools::mesh panels_mesh, outline_mesh;

   panels_mesh.SetOffset(offset);
   outline_mesh.SetOffset(offset);
   panels_mesh.SetColor(csg::Color3::FromString("#00DFFC"));
   outline_mesh.SetColor(csg::Color3::FromString("#005FFB"));
   csg::RegionTools3().ForEachPlane(model, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      csg::Region2f outline = csg::RegionTools2().GetInnerBorder(plane, thickness);
      csg::Region2f panels = ToFloat(plane) - outline;
      outline_mesh.AddRegion(outline, csg::ToFloat(pi));
      panels_mesh.AddRegion(panels, csg::ToFloat(pi));
   });
   AddDynamicMeshNode(group, panels_mesh, material_path, 0);
   AddDynamicMeshNode(group, outline_mesh, material_path, 0);

   return group;
}

H3DNodeUnique Pipeline::CreateVoxelNode(H3DNode parent,
                                        csg::Region3 const& model,
                                        std::string const& material,
                                        csg::Point3f const& offset,
                                        int userFlags)
{
   csg::mesh_tools::mesh mesh;
   csg::RegionToMesh(model, mesh, offset);
   return AddDynamicMeshNode(parent, mesh, material, userFlags);
}

void Pipeline::AddDesignationStripes(csg::mesh_tools::mesh& m, csg::Region2 const& panels)
{   
   float y = 0;
   for (csg::Rect2 const& c: panels) {      
      csg::Rect2f cube = ToFloat(c);
      csg::Point2f size = cube.GetSize();
      for (float i = 0; i < size.y; i ++) {
         // xxx: why do we have to use a clockwise winding here?
         float x1 = std::min(i + 1.0f, size.x);
         float x2 = std::min(i + 0.5f, size.x);
         float y1 = std::max(i + 1.0f - size.x, 0.0f);
         float y2 = std::max(i + 0.5f - size.x, 0.0f);
         csg::Point3f points[] = {
            csg::Point3f(cube.min.x, y, cube.min.y + i + 1.0f),
            csg::Point3f(cube.min.x + x1, y, cube.min.y + y1),
            csg::Point3f(cube.min.x + x2, y, cube.min.y + y2),
            csg::Point3f(cube.min.x, y, cube.min.y + i + 0.5f),
         };
         m.AddFace(points, csg::Point3f::unitY, csg::Color3(0, 0, 0));
      }
      for (float i = 0; i < size.x; i ++) {
         // xxx: why do we have to use a clockwise winding here?
         float x0 = i + 0.5f;
         float x1 = i + 1.0f;
         float x2 = std::min(x1 + size.y, size.x);
         float x3 = std::min(x0 + size.y, size.x);
         float y2 = -std::min(x2 - x1, size.y);
         float y3 = -std::min(x3 - x0, size.y);
         csg::Point3f points[] = {
            csg::Point3f(cube.min.x + x0, y, cube.max.y),
            csg::Point3f(cube.min.x + x1, y, cube.max.y),
            csg::Point3f(cube.min.x + x2, y, cube.max.y + y2),
            csg::Point3f(cube.min.x + x3, y, cube.max.y + y3),
         };
         m.AddFace(points, csg::Point3f::unitY, csg::Color3(0, 0, 0));
      }
   }
}

void Pipeline::AddDesignationBorder(csg::mesh_tools::mesh& m, csg::EdgeMap2& edgemap)
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
      float min_t = dash.min[t];
      float max_t = dash.max[t];

      // Min corner...
      if (edge.min->accumulated_normals.Length() > 1) {
         dash.min[t] = min_t;
         dash.max[t] = dash.min[t] + 0.75f;
         m.AddRect(dash, ToFloat(pi));
         min_t += 1.0f;
      }
      // Max corner...
      if (edge.max->accumulated_normals.Length() > 1) {
         dash.max[t] = max_t;
         dash.min[t] = dash.max[t] - 0.75f;
         m.AddRect(dash, ToFloat(pi));
         max_t -= 1.0f;
      }
      // Range...
      for (float v = min_t; v < max_t; v++) {
         dash.min[t] = v + 0.25f;
         dash.max[t] = v + 0.75f;
         m.AddRect(dash, ToFloat(pi));
      }
   }
}

H3DNodeUnique
Pipeline::CreateDesignationNode(H3DNode parent,
                                csg::Region2 const& plane,
                                csg::Color3 const& outline_color,
                                csg::Color3 const& stripes_color)
{
   csg::RegionTools2 tools;

   csg::mesh_tools::mesh outline_mesh;
   csg::mesh_tools::mesh stripes_mesh;

   // flip the normal, since this is the bottom face
   outline_mesh.SetColor(outline_color);
   stripes_mesh.SetColor(stripes_color);

   csg::EdgeMap2 edgemap = csg::RegionTools2().GetEdgeMap(plane);
   AddDesignationBorder(outline_mesh, edgemap);
   AddDesignationStripes(stripes_mesh, plane);

   H3DNode group = h3dAddGroupNode(parent, "designation group node");
   H3DNode stripes = AddDynamicMeshNode(group, stripes_mesh, "materials/designation/stripes.material.xml", 0);
   h3dSetNodeParamI(stripes, H3DModel::UseCoarseCollisionBoxI, 1);
   h3dSetNodeParamI(stripes, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(stripes, H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(stripes, H3DModel::PolygonOffsetF, 1, -.01f);

   H3DNode outline = AddDynamicMeshNode(group, outline_mesh, "materials/designation/outline.material.xml", 0);
   h3dSetNodeParamI(outline, H3DModel::UseCoarseCollisionBoxI, 1);
   h3dSetNodeParamI(outline, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(outline, H3DModel::PolygonOffsetF, 0, -1.0);
   h3dSetNodeParamF(outline, H3DModel::PolygonOffsetF, 1, -.01f);

   return group;
}

voxel::QubicleFile* Pipeline::LoadQubicleFile(std::string const& uri)
{   
   auto& r = res::ResourceManager2::GetInstance();

   // Keep a cache of the most recently loaded qubicle files so we don't hammer the
   // filesystem (where "recent" currently means, "ever").  Be sure to store them by
   // the canonical path so we don't get duplicates (e.g. stonehearth:foo vs.
   // stonehearth/entities/foo/foo.qb)

   std::string const& path = r.ConvertToCanonicalPath(uri, nullptr);
   auto i = qubicle_files_.find(path);
   if (i != qubicle_files_.end()) {
      return i->second.get();
   }
   voxel::QubicleFilePtr f = std::make_shared<voxel::QubicleFile>(path);
   std::ifstream input;
   std::shared_ptr<std::istream> is = res::ResourceManager2::GetInstance().OpenResource(uri);
   (*is) >> *f;
   qubicle_files_[path] = f;

   return f.get();
}
