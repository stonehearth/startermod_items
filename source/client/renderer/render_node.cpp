#include "pch.h"
#include "resources/res_manager.h"
#include "radiant_macros.h"
#include "radiant_stdutil.h"
#include "render_node.h"
#include "Horde3DUtils.h"
#include "pipeline.h"

using namespace ::radiant;
using namespace ::radiant::client;

template <typename X, typename Y>
struct PairHash {
public:
   size_t operator()(std::pair<X, Y> const& x) const {
      return std::hash<X>()(x.first) ^ std::hash<Y>()(x.second);
   }
};

#define RN_LOG(level)   LOG(renderer.render_node, level)

static int nextId = 1;

RenderNode RenderNode::CreateMeshNode(H3DNode parent, GeometryInfo const& geo)
{
   std::string modelName = BUILD_STRING("model" << nextId++);
   std::string meshName = BUILD_STRING("mesh" << nextId++);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/skysphere.material.xml");
   H3DNode node = h3dAddModelNode(parent, modelName.c_str(), geo.geo.get());

   int indexCount = geo.indexIndicies[geo.levelCount - 1];
   int vertexCount = geo.vertexIndices[geo.levelCount - 1];
   H3DNode meshNode = h3dAddMeshNode(node, meshName.c_str(), mat.get(), 0, indexCount, 0, vertexCount - 1);

   return RenderNode(node, meshNode, geo.geo, mat);
}


RenderNode RenderNode::CreateVoxelNode(H3DNode parent, GeometryInfo const& geo)
{
   std::string modelName = BUILD_STRING("model" << nextId++);
   std::string meshName = BUILD_STRING("mesh" << nextId++);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/voxel.material.xml");
   H3DNode node = h3dAddVoxelModelNode(parent, modelName.c_str(), geo.geo.get());
   H3DNode meshNode = h3dAddVoxelMeshNode(node, meshName.c_str(), mat.get());

   return RenderNode(node, meshNode, geo.geo, mat);
}

RenderNode RenderNode::CreateObjFileNode(H3DNode parent, std::string const& uri)
{
   res::ResourceManager2 &r = res::ResourceManager2::GetInstance();
   std::string const& path = r.ConvertToCanonicalPath(uri, ".obj");

   ResourceCacheKey key;
   key.AddElement("obj filename", path);

   GeometryInfo geo;
   if (!Pipeline::GetInstance().GetSharedGeometry(key, geo)) {
      std::shared_ptr<std::istream> is = r.OpenResource(path);
      if (!is) {
         return RenderNode(); // xxx: return invalid object node (a companion cube?)
      }
      ConvertObjFileToGeometry(*is, geo);
      Pipeline::GetInstance().SetSharedGeometry(key, geo);
   }
   return CreateMeshNode(parent, geo);
}

RenderNode RenderNode::CreateCsgMeshNode(H3DNode parent, csg::mesh_tools::mesh const& m)
{
   GeometryInfo geo;

   geo.vertexIndices[1] = m.vertices.size();
   geo.indexIndicies[1] = m.indices.size();
   geo.levelCount = 1;

   ConvertVoxelDataToGeometry((VoxelGeometryVertex *)m.vertices.data(), (uint *)m.indices.data(), geo);
   return CreateVoxelNode(parent, geo);
}

RenderNode RenderNode::CreateSharedCsgMeshNode(H3DNode parent, ResourceCacheKey const& key, CreateMeshLodLevelFn create_mesh_fn)
{   
   GeometryInfo geo;
   if (!Pipeline::GetInstance().GetSharedGeometry(key, geo)) {

      RN_LOG(7) << "creating new geometry for " << key.GetDescription();

      csg::mesh_tools::mesh m;
      for (int i = 1; i <= MAX_LOD_LEVELS; i++) {
         create_mesh_fn(m, i);
         geo.vertexIndices[i] = m.vertices.size();
         geo.indexIndicies[i] = m.indices.size();
      }
      geo.levelCount = MAX_LOD_LEVELS;
      ConvertVoxelDataToGeometry((VoxelGeometryVertex *)m.vertices.data(), (uint *)m.indices.data(), geo);
      Pipeline::GetInstance().SetSharedGeometry(key, geo);
   }
   return CreateVoxelNode(parent, geo);
}

RenderNode::RenderNode() :
   _id(nextId++)
{
}

RenderNode::RenderNode(H3DNode node) :
   _id(nextId++),
   _node(node)
{
}

RenderNode::RenderNode(H3DNode node, H3DNode mesh, SharedGeometry geo, SharedMaterial mat) :
   _id(nextId++),
   _node(node),
   _meshNode(mesh),
   _geometry(geo),
   _material(mat)
{

}

RenderNode::~RenderNode()
{
}

RenderNode& RenderNode::SetUserFlags(int flags)
{
   // why are flags on the mesh and not the model?
   h3dSetNodeParamI(_meshNode.get(), H3DNodeParams::UserFlags, flags);
   return *this;
}

RenderNode& RenderNode::SetGeometry(SharedGeometry geo)
{
   _geometry = geo;
   return *this;
}

RenderNode& RenderNode::SetMaterial(std::string const& material)
{
   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial(material);
   return SetMaterial(mat);
}

RenderNode& RenderNode::SetMaterial(SharedMaterial material)
{
   _material = material;
   ApplyMaterial();
   return *this;
}

RenderNode& RenderNode::SetTransform(csg::Point3f const& pos, csg::Point3f const& rot, csg::Point3f const& scale)
{
   h3dSetNodeTransform(_node.get(), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, scale.x, scale.y, scale.z);
   return *this;
}

RenderNode& RenderNode::AddChild(const RenderNode& r)
{
   _children.push_back(r);
   return *this;
}

void RenderNode::Destroy()
{
   _children.clear();
   _node.reset();
   _meshNode.reset();
   _geometry.reset();
   _material.reset();
}

RenderNode& RenderNode::SetOverrideMaterial(SharedMaterial overrideMaterial)
{
   _overrideMaterial = overrideMaterial;
   ApplyMaterial();
   return *this;
}

void RenderNode::ApplyMaterial()
{
   if (_meshNode) {
      H3DRes mat = _material.get();
      if (_overrideMaterial.get()) {
         mat = _overrideMaterial.get();
      }
      h3dSetNodeParamI(_meshNode.get(), H3DVoxelMeshNodeParams::MatResI, mat);
   }
}

void RenderNode::ConvertVoxelDataToGeometry(VoxelGeometryVertex *vertices, uint *indices, GeometryInfo& geo)
{
   std::string geoName = BUILD_STRING("geo" << nextId++);

   geo.geo = h3dutCreateVoxelGeometryRes(geoName.c_str(),
                                         vertices,
                                         geo.vertexIndices,
                                         indices,
                                         geo.indexIndicies,
                                         geo.levelCount);
}

void RenderNode::ConvertObjFileToGeometry(std::istream& stream, GeometryInfo &geo)
{
   csg::Point3f pt;
   std::vector<csg::Point3f> obj_verts;
   std::vector<csg::Point3f> obj_norms;
   std::unordered_map<std::pair<int, int>, int, PairHash<int, int>> vertex_map;
   std::vector<float> vertices;
   std::vector<unsigned int> indices;
   std::vector<short> normalData;

   std::string line;

   auto packNormal = [](float coord) -> short {
      return static_cast<short>(coord * 32767);
   };

   auto addIndex = [&](int vi, int ni) {
      std::pair<int, int> key(vi, ni);
      auto i = vertex_map.find(key);
      if (i != vertex_map.end()) {
         indices.push_back(i->second);
      } else {
         // create a new point
         csg::Point3f const& v = obj_verts[vi];
         csg::Point3f const& n = obj_norms[ni];

         vertices.push_back(v.x);
         vertices.push_back(v.y);
         vertices.push_back(v.z);

         normalData.push_back(packNormal(n.x));
         normalData.push_back(packNormal(n.y));
         normalData.push_back(packNormal(n.z));

         int index = vertex_map.size();
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
            int v[3], n[3]; 
            if (sscanf(line.c_str(), "%*s v%d//vn%d v%d//vn%d v%d//vn%d", &v[0], &n[0], &v[1], &n[1], &v[2], &n[2]) == 6) {
               addIndex(v[0], n[0]);
               addIndex(v[1], n[1]);
               addIndex(v[2], n[2]);
            }
         }
      }
   }
   
   std::string geoName = BUILD_STRING("geo" << nextId++);
   geo.vertexIndices[1] = vertices.size();
   geo.indexIndicies[1] = indices.size();
   geo.levelCount = 1;
   geo.geo = h3dutCreateGeometryRes(geoName.c_str(), vertices.size(), indices.size(), vertices.data(),
                                    indices.data(), normalData.data(), nullptr, nullptr, nullptr, nullptr);
}
