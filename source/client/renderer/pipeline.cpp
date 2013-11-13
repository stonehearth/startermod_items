#include "pch.h"
#include "pipeline.h"
#include "metrics.h"
#include "Horde3DUtils.h"
#include "renderer.h"
#include "lib/voxel/qubicle_file.h"
#include "resources/res_manager.h"
#include "csg/region_tools.h"
#include <fstream>
#include <unordered_map>

using namespace ::radiant;

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

Pipeline::Pipeline()
{
   //_actorMaterial = Ogre::MaterialManager::getSingleton().load("Tessaract/ActorMaterialTemplate", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
   //_tileMaterial = Ogre::MaterialManager::getSingleton().load("Tessaract/TileMaterialTemplate", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
   // ASSERT(!_actorMaterial.isNull());
   orphaned_ = H3DNodeUnique(h3dAddGroupNode(H3DRootNode, "pipeline orphaned nodes"));
   h3dSetNodeFlags(orphaned_.get(), H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);

   float offscreen = -100000.0f;
   h3dSetNodeTransform(orphaned_.get(), offscreen, offscreen, offscreen, 0, 0, 0, 1, 1, 1);
}

Pipeline::~Pipeline()
{
}

// From: http://mikolalysenko.github.com/MinecraftMeshes2/js/greedy.js

H3DNodeUnique Pipeline::AddMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m, H3DNode* mesh)
{
   static int unique = 0;
   std::string name = "mesh data ";

   H3DRes res = h3dutCreateVoxelGeometryRes((name + stdutil::ToString(unique++)).c_str(), (VoxelGeometryVertex *)m.vertices.data(), m.vertices.size(), (uint *)m.indices.data(), m.indices.size());
   H3DRes matRes = h3dAddResource(H3DResTypes::Material, "terrain/default_material.xml", 0);
   H3DNode model_node = h3dAddVoxelModelNode(parent, (name + stdutil::ToString(unique++)).c_str(), res);
   H3DNode mesh_node = h3dAddVoxelMeshNode(model_node, (name + stdutil::ToString(unique++)).c_str(), matRes, 0, m.indices.size(), 0, m.vertices.size() - 1);

   if (mesh) {
      *mesh = mesh_node;
   }

   // xxx: how do res, matRes, and mesh_node get deleted? - tony
   return H3DNodeUnique(model_node);
}

H3DNodeUnique Pipeline::AddQubicleNode(H3DNode parent, const voxel::QubicleMatrix& m, const csg::Point3f& origin, H3DNode* mesh)
{
   std::vector<VoxelGeometryVertex> vertices;
   std::vector<uint32> indices;

   //std::unordered_map<Vertex, int> vertcache;

   // Super greedy...   
 
   const csg::Point3& size = m.GetSize();
   const csg::Point3f matrixPosition((float)m.position_.x, (float)m.position_.y, (float)m.position_.z);

   uint32* mask = new uint32[size.x * size.y * size.z];

   for (const auto &edge : qubicle_node_edges__) {
      csg::Point3 pt;
      for (int i = 0; i < size[edge.i]; i++) {
         int maskSize = 0, offset = 0;

         // compute a mask of the current plane
         for (int v = 0; v < size[edge.v]; v++) {
            for (int u = 0; u < size[edge.u]; u++) {
               pt[edge.i] = i;
               pt[edge.u] = u;
               pt[edge.v] = v;

               uint32 c = m.At(pt.x, pt.y, pt.z);
               uint32 alpha = (c >> 24);
               if ((alpha & edge.mask) != 0) {
                  // make alpha fully opaque so we can use integer comparision.  setting
                  // alpha to 0 makes black (#000000) fully transparent, which isn't what
                  // we want.
                  mask[offset] = c | 0xFF000000;
                  maskSize++;
               } else {
                  mask[offset] = 0;
               }
               offset++;
            }
         }
         ASSERT(offset <= size[edge.u] * size[edge.v]);

         // Suck rectangles out of the mask as greedily as possible
         offset = 0;
         for (int v = 0; v < size[edge.v] && maskSize > 0; v++) {
            for (int u = 0; u < size[edge.u] && maskSize > 0; ) {
               uint32 c = mask[offset];
               if (!c) {
                  u++;
                  offset++;
               } else {
                  int w, h, t;
                  // grab the biggest rectangle we can...
                  for (w = 1; u + w < size[edge.u]; w++) {
                     if (mask[offset + w] != c) {
                        break;
                     }
                  }
                  // grab the biggest height we can...
                  for (h = 1; v + h < size[edge.v]; h++) {
                     for (t = 0; t < w; t++) {
                        if (mask[offset + t + h*size[edge.u]] != c) {
                           break;
                        }
                     }
                     if (t != w) {
                        break;
                     }
                  }
                  // add a quad for the w/h rectangle...
                  csg::Point3f min = edge.normal / 2;
                  min[edge.i] += (float)i;
                  min[edge.u] += (float)u - 0.5f;
                  min[edge.v] += (float)v - 0.5f;

                  // Fudge factor to accout for the different origins between
                  // the .qb file and the skeleton...
                  min.x += 0.5;
                  min.z += 0.5;
                  min.y += 0.5;

               
#define COPY_VEC(a, b) ((a)[0] = (b)[0]), ((a)[1] = (b)[1]), ((a)[2] = (b)[2])

                  VoxelGeometryVertex vertex;
                  COPY_VEC(vertex.normal, edge.normal);
                  csg::Color3 col = m.GetColor(c);
                  vertex.color[0] = col.r / 255.0f;
                  vertex.color[1] = col.g / 255.0f;
                  vertex.color[2] = col.b / 255.0f;

                  // UGGGGGGGGGGGGGE
                  int voffset = vertices.size();
                  
                  // 3DS Max uses a z-up, right handed origin....
                  // The voxels in the matrix are stored y-up, left-handed...
                  // Convert the max origin to y-up, right handed...
                  csg::Point3f yUpOrigin(origin.x, origin.z, origin.y); 

                  // Now convert to y-up, left-handed
                  csg::Point3f yUpLHOrigin(yUpOrigin.x, yUpOrigin.y, -yUpOrigin.z);

#if 0
                  // The order of the voxels in the matric have the front at
                  // the positive z-side of the model.  Flip it so the actor is
                  // looking down the negative z axis
#endif
                  COPY_VEC(vertex.pos, (min + matrixPosition) - yUpLHOrigin);
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] += h;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.u] += w;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] -= h;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  // xxx - the whole conversion bit above can be greatly optimized,
                  // but I don't want to touch it now that it's workin'!

                  if (edge.mask == voxel::RIGHT_MASK || edge.mask == voxel::FRONT_MASK || edge.mask == voxel::BOTTOM_MASK) {
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset + 1);
                     indices.push_back(voffset);

                     indices.push_back(voffset + 3);
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset);
                  } else {
                     indices.push_back(voffset);
                     indices.push_back(voffset + 1);
                     indices.push_back(voffset + 2);

                     indices.push_back(voffset);
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset + 3);
                  }

                  // ...zero out the mask
                  for (int k = v; k < v + h; k++) {
                     for (int j = u; j < u + w; j++) {
                        mask[j + size[edge.u] * k ] = 0;
                     }
                  }

                  // ...and move past this rectangle
                  u += w;
                  offset += w;
                  maskSize -= w * h;

                  ASSERT(offset <= size[edge.u] * size[edge.v]);
                  ASSERT(maskSize >= 0);
               }
            }
         }
      }
   }
   delete[] mask;

   static int unique = 0;
   std::string name = "qubicle data ";
   H3DRes geoRes = h3dutCreateVoxelGeometryRes((name + stdutil::ToString(unique++)).c_str(), vertices.data(), vertices.size(), indices.data(), indices.size());
   H3DRes matRes = h3dAddResource(H3DResTypes::Material, "terrain/default_material.xml", 0);
   H3DNode model_node = h3dAddVoxelModelNode(parent, (name + stdutil::ToString(unique++)).c_str(), geoRes);
   H3DNode mesh_node = h3dAddVoxelMeshNode(model_node, (name + stdutil::ToString(unique++)).c_str(), matRes, 0, indices.size(), 0, vertices.size() - 1);

   if (mesh) {
      *mesh = mesh_node;
   }
   return H3DNodeUnique(model_node);
}


H3DNode Pipeline::CreateModel(H3DNode parent,
                              csg::mesh_tools::mesh const& mesh,
                              std::string const& material_path)
{
   H3DNode mesh_node;

   H3DNodeUnique model_node = Pipeline::GetInstance().AddMeshNode(parent, mesh, &mesh_node);
   if (!material_path.empty()) {
      H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
      h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);
   }

   H3DNode node = model_node.get();
   model_node.release();

   return node;
}

H3DNodeUnique Pipeline::CreateBlueprintNode(H3DNode parent,
                                            csg::Region3 const& model,
                                            float thickness,
                                            std::string const& material_path)
{
   static int unique = 1;

   H3DNode group = h3dAddGroupNode(parent, BUILD_STRING("blueprint node " << unique++).c_str());

   csg::mesh_tools::mesh panels_mesh, outline_mesh;
   panels_mesh.SetColor(csg::Color3::FromString("#00DFFC"));
   outline_mesh.SetColor(csg::Color3::FromString("#005FFB"));
   csg::RegionTools3().ForEachPlane(model, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      csg::Region2f outline = csg::RegionTools2().GetInnerBorder(plane, thickness);
      csg::Region2f panels = ToFloat(plane) - outline;
      outline_mesh.AddRegion(outline, csg::ToFloat(pi));
      panels_mesh.AddRegion(panels, csg::ToFloat(pi));
   });
   CreateModel(group, panels_mesh, material_path);
   CreateModel(group, outline_mesh, material_path);

   return group;
}

H3DNodeUnique Pipeline::CreateVoxelNode(H3DNode parent,
                                        csg::Region3 const& model,
                                        std::string const& material_path)
{
   static int unique = 1;

   csg::mesh_tools::mesh mesh;
   csg::RegionTools3().ForEachPlane(model, [&](csg::Region2 const& plane, csg::PlaneInfo3 const& pi) {
      mesh.AddRegion(plane, pi);
   });
   return CreateModel(parent, mesh, material_path);
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
   H3DNode stripes = CreateModel(group, stripes_mesh, "materials/designation/stripes.material.xml");
   h3dSetNodeParamI(stripes, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(stripes, H3DModel::PolygonOffsetF, 0, -.01f);
   h3dSetNodeParamF(stripes, H3DModel::PolygonOffsetF, 1, -.01f);

   H3DNode outline = CreateModel(group, outline_mesh, "materials/designation/outline.material.xml");
   h3dSetNodeParamI(outline, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(outline, H3DModel::PolygonOffsetF, 0, -.01f);
   h3dSetNodeParamF(outline, H3DModel::PolygonOffsetF, 1, -.01f);

   return group;
}

Pipeline::NamedNodeMap Pipeline::LoadQubicleFile(std::string const& uri)
{   
   NamedNodeMap result;

   // xxx: no.  Make a qubicle resource type so they only get loaded once, ever.
   voxel::QubicleFile f;
   std::ifstream input;

   std::shared_ptr<std::istream> is = res::ResourceManager2::GetInstance().OpenResource(uri);
   (*is) >> f;

   for (const auto& entry : f) {
      // Qubicle requires that every matrix in the file have a unique name.  While authoring,
      // if you copy/pasta a matrix, it will rename it from matrixName to matrixName_2 to
      // dismabiguate them.  Ignore everything after the _ so we don't make authors manually
      // rename every single part when this happens.
      std::string matrixName = entry.first;
      result[matrixName] = AddQubicleNode(orphaned_.get(), entry.second, csg::Point3f(0, 0, 0));
   }

   return result;
}
