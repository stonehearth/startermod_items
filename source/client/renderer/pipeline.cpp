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
   H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
   h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);

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
      csg::Region2f outline = ToFloat(plane);
      csg::Region2f panels = csg::RegionTools2().Inset(plane, thickness);
      outline -= panels;
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
      mesh.AddRegion(plane, pi); // xxx: make an integer version!
   });
   return CreateModel(parent, mesh, material_path);
}

Pipeline::NamedNodeMap Pipeline::LoadQubicleFile(std::string const& uri)
{   
   NamedNodeMap result;

   // xxx: no.  Make a qubicle resource type so they only get loaded once, ever.
   voxel::QubicleFile f;
   std::ifstream input;

   res::ResourceManager2::GetInstance().OpenResource(uri, input);
   input >> f;

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
