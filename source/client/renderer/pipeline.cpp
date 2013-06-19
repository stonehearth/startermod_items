#include "pch.h"
#include "pipeline.h"
#include "metrics.h"
#include "geometry_generator.h"
#include "Horde3DUtils.h"
#include "texture_color_mapper.h"
#include "om/grid/grid.h"
#include "renderer.h"
#include "qubicle_file.h"
#include "resources/res_manager.h"
#include <fstream>
#include <unordered_map>

using namespace ::radiant;

namespace metrics = ::radiant::metrics;
using ::radiant::math3d::ipoint3;

using namespace ::radiant;
using namespace ::radiant::client;

DEFINE_SINGLETON(Pipeline);

std::unique_ptr<TextureColorMapper> TextureColorMapper::mapper_;

Pipeline::Pipeline()
{
   //_actorMaterial = Ogre::MaterialManager::getSingleton().load("Tessaract/ActorMaterialTemplate", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
   //_tileMaterial = Ogre::MaterialManager::getSingleton().load("Tessaract/TileMaterialTemplate", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
   // ASSERT(!_actorMaterial.isNull());
   orphaned_ = h3dAddGroupNode(H3DRootNode, "pipeline orphaned nodes");
   h3dSetNodeFlags(orphaned_, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);

   float offscreen = -100000.0f;
   h3dSetNodeTransform(orphaned_, offscreen, offscreen, offscreen, 0, 0, 0, 1, 1, 1);
}

Pipeline::~Pipeline()
{
}

H3DNode Pipeline::GetTileEntity(const om::GridPtr grid, om::GridTilePtr tile, H3DRes parent)
{
#if 1
   GeometryGenerator geometry(grid, tile);

   H3DNode result = 0;
   if (geometry.Generate()) {
      auto& mapper = TextureColorMapper::GetInstance();
      static int nameOffset = 0;
      std::ostringstream name;
      name << "Tile " << tile->GetObjectId() << " " << nameOffset++;

      GeometryResource geo = CreateMesh(name.str(), geometry.GetGeometry());

      result = h3dAddModelNode(parent, "blocks", geo.geometry);
      H3DNode mesh = h3dAddMeshNode(result , "mesh", mapper.GetMaterial(), 0, geo.numIndices, 0, geo.numVertices - 1);
   }
   
   return result;
#else
   //if (tile->isEmpty()) {
   //   return NULL;
   //}

   GeometryGenerator geometry(grid, tile);

   H3DNode result = 0;
   if (geometry.Generate()) {
      TextureColorMapper::GetInstance();
      auto& mapper = TextureColorMapper::GetInstance();
      auto& vertices = geometry.GetVertices();
      auto& indices = geometry.GetIndices();
      ostringstream name;
      static int nameOffset = 0;
      name << "Tile " << tile->GetObjectId() << " " << nameOffset++;

      int numVertices = vertices.size();
      int numTriangleIndices = indices.size();

      vector<float> posData;
      vector<short> normalData;
      vector<float> texData1;
      for (auto& v : vertices) {
         posData.push_back(v.x);
         posData.push_back(v.y);
         posData.push_back(v.z);
         normalData.push_back((short)(v.nx * 32767.0f));
         normalData.push_back((short)(v.ny * 32767.0f));
         normalData.push_back((short)(v.nz * 32767.0f));

         std::pair<float, float> uv = mapper.MapColor(v.color);
         texData1.push_back(uv.first);
         texData1.push_back(uv.second);
      }
      H3DNode geom = h3dutCreateGeometryRes(name.str().c_str(), numVertices, numTriangleIndices, 
                                            &posData[0], (unsigned int *)&indices[0], &normalData[0],
								                    NULL, NULL, &texData1[0], NULL);

      result = h3dAddModelNode(parent, "blocks", geom);
      H3DNode mesh = h3dAddMeshNode(result , "mesh", mapper.GetMaterial(), 0, numTriangleIndices, 0, numVertices - 1);
   }
   
   return result;
#endif
}


Pipeline::GeometryResource Pipeline::CreateMesh(std::string name, const Geometry& geo)
{
   auto& mapper = TextureColorMapper::GetInstance();

   int numVertices = geo.vertices.size();
   int numTriangleIndices = geo.indices.size();

   std::vector<float> posData;
   std::vector<short> normalData;
   std::vector<float> texData1;
   for (auto& v : geo.vertices) {
      posData.push_back(v.pos.x);
      posData.push_back(v.pos.y);
      posData.push_back(v.pos.z);
      normalData.push_back((short)(v.normal.x * 32767.0f));
      normalData.push_back((short)(v.normal.y * 32767.0f));
      normalData.push_back((short)(v.normal.z * 32767.0f));

      std::pair<float, float> uv = mapper.MapColor(v.color);
      texData1.push_back(uv.first);
      texData1.push_back(uv.second);
   }
#if 1
   GeometryResource result;
   result.geometry = h3dutCreateGeometryRes(name.c_str(), numVertices, numTriangleIndices, 
                                            &posData[0], (unsigned int *)&geo.indices[0], &normalData[0],
								                    NULL, NULL, &texData1[0], NULL);
   result.numIndices = numTriangleIndices;
   result.numVertices = numVertices;
   
   return result;
#else
   H3DNode geom = h3dutCreateGeometryRes(name.c_str(), numVertices, numTriangleIndices, 
                                         &posData[0], (unsigned int *)&indices[0], &normalData[0],
								                 NULL, NULL, &texData1[0], NULL);

   H3DNode result = h3dAddModelNode(parent, "blocks", geom);
   H3DNode mesh = h3dAddMeshNode(result , "mesh", mapper.GetMaterial(), 0, numTriangleIndices, 0, numVertices - 1);

   return result;
#endif
}

// From: http://mikolalysenko.github.com/MinecraftMeshes2/js/greedy.js

Pipeline::Geometry Pipeline::OptimizeQubicle(const QubicleMatrix& m, const csg::Point3f& origin)
{
   Pipeline::Geometry geo;
   //std::unordered_map<Vertex, int> vertcache;

   // Super greedy...
   
   static const struct {
      int i, u, v, mask;
      csg::Point3f normal;
   } edges[] = {
      { 2, 0, 1, FRONT_MASK,  csg::Point3f( 0,  0,  1) },
      { 2, 0, 1, BACK_MASK,   csg::Point3f( 0,  0, -1) },
      { 1, 0, 2, TOP_MASK,    csg::Point3f( 0,  1,  0) },
      { 1, 0, 2, BOTTOM_MASK, csg::Point3f( 0, -1,  0) },
      { 0, 2, 1, RIGHT_MASK,  csg::Point3f(-1,  0,  0) },
      { 0, 2, 1, LEFT_MASK,   csg::Point3f( 1,  0,  0) },
   };
   
   const csg::Point3& size = m.GetSize();
   const csg::Point3f matrixPosition((float)m.position_.x, (float)m.position_.y, (float)m.position_.z);

   uint32* mask = new uint32[size.x * size.y * size.z];

   for (const auto &edge : edges) {
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

                  Vertex vertex;
                  vertex.normal = edge.normal;
                  vertex.color = m.GetColor(c);

                  // UGGGGGGGGGGGGGE
                  int voffset = geo.vertices.size();
                  
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
                  vertex.pos = (min + matrixPosition) - yUpLHOrigin;
                  geo.vertices.push_back(vertex);
                  //geo.vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] += h;
                  geo.vertices.push_back(vertex);
                  //geo.vertices.back().pos.z *= -1;

                  vertex.pos[edge.u] += w;
                  geo.vertices.push_back(vertex);
                  //geo.vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] -= h;
                  geo.vertices.push_back(vertex);
                  //geo.vertices.back().pos.z *= -1;

                  // xxx - the whole conversion bit above can be greatly optimized,
                  // but I don't want to touch it now that it's workin'!

                  if (edge.mask == RIGHT_MASK || edge.mask == FRONT_MASK || edge.mask == BOTTOM_MASK) {
                     geo.indices.push_back(voffset + 2);
                     geo.indices.push_back(voffset + 1);
                     geo.indices.push_back(voffset);

                     geo.indices.push_back(voffset + 3);
                     geo.indices.push_back(voffset + 2);
                     geo.indices.push_back(voffset);
                  } else {
                     geo.indices.push_back(voffset);
                     geo.indices.push_back(voffset + 1);
                     geo.indices.push_back(voffset + 2);

                     geo.indices.push_back(voffset);
                     geo.indices.push_back(voffset + 2);
                     geo.indices.push_back(voffset + 3);
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
   delete mask;

   return geo;
}

Pipeline::NamedNodeMap Pipeline::LoadQubicleFile(std::string const& uri)
{   
   NamedNodeMap result;

   // xxx: no.  Make a qubicle resource type so they only get loaded once, ever.
   QubicleFile f;
   std::ifstream input;

   resources::ResourceManager2::GetInstance().OpenResource(uri, input);
   input >> f;

   for (const auto& entry : f) {
      // Qubicle requires that every matrix in the file have a unique name.  While authoring,
      // if you copy/pasta a matrix, it will rename it from matrixName to matrixName_2 to
      // dismabiguate them.  Ignore everything after the _ so we don't make authors manually
      // rename every single part when this happens.
      std::string matrixName = entry.first;

      Pipeline::Geometry geo = OptimizeQubicle(entry.second, csg::Point3f(0, 0, 0));

      auto& mapper = TextureColorMapper::GetInstance();
      // Geometry resource names must be unique.  WAT????
      Pipeline::GeometryResource gr = CreateMesh(uri + "?matrix=" + matrixName, geo);

      H3DNode node = h3dAddModelNode(orphaned_, "blocks", gr.geometry);
      H3DNode mesh = h3dAddMeshNode(node , "mesh", mapper.GetMaterial(), 0, gr.numIndices, 0, gr.numVertices - 1);

      result[matrixName] = node;
   }

   return result;
}
