#include "pch.h"
#include <unordered_set>
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
H3DNode RenderNode::_unparentedRenderNode;
std::unordered_set<H3DNode> ownedNodes;

RenderNodePtr RenderNode::CreateGroupNode(H3DNode parent, std::string const& name)
{
   return std::make_shared<RenderNode>(h3dAddGroupNode(parent, name.c_str()));
}

RenderNodePtr RenderNode::CreateMeshNode(H3DNode parent, GeometryInfo const& geo)
{
   std::string modelName = BUILD_STRING("model" << nextId++);
   std::string meshName = BUILD_STRING("mesh" << nextId++);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/obj.material.xml");
   H3DNode node = h3dAddModelNode(parent, modelName.c_str(), geo.geo.get());

   int indexCount = geo.indexIndicies[geo.levelCount];
   int vertexCount = geo.vertexIndices[geo.levelCount];
   H3DNode meshNode = h3dAddMeshNode(node, meshName.c_str(), mat.get(), 0, indexCount, 0, vertexCount - 1);

   return std::make_shared<RenderNode>(node, meshNode, geo.geo, mat);
}

RenderNodePtr RenderNode::CreateVoxelNode(H3DNode parent, GeometryInfo const& geo)
{
   std::string modelName = BUILD_STRING("model" << nextId++);
   std::string meshName = BUILD_STRING("mesh" << nextId++);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/voxel.material.xml");
   H3DNode node = h3dAddVoxelModelNode(parent, modelName.c_str(), geo.geo.get());
   H3DNode meshNode = h3dAddVoxelMeshNode(node, meshName.c_str(), mat.get());
   if (geo.unique) {
      h3dSetNodeParamI(meshNode, H3DVoxelMeshNodeParams::NoInstancingI, true);
   }
   return std::make_shared<RenderNode>(node, meshNode, geo.geo, mat);
}

RenderNodePtr RenderNode::CreateObjNode(H3DNode parent, std::string const& uri)
{
   res::ResourceManager2 &r = res::ResourceManager2::GetInstance();
   std::string const& path = r.ConvertToCanonicalPath(uri, ".obj");

   ResourceCacheKey key;
   key.AddElement("obj filename", path);

   GeometryInfo geo;
   if (!Pipeline::GetInstance().GetSharedGeometry(key, geo)) {
      std::shared_ptr<std::istream> is = r.OpenResource(path);
      if (!is) {
         return nullptr;
      }
      ConvertObjFileToGeometry(*is, geo);
      Pipeline::GetInstance().SetSharedGeometry(key, geo);
   }
   return CreateMeshNode(parent, geo);
}

RenderNodePtr RenderNode::CreateCsgMeshNode(H3DNode parent, csg::mesh_tools::mesh const& m)
{
   GeometryInfo geo;

   geo.vertexIndices[1] = m.vertices.size();
   geo.indexIndicies[1] = m.indices.size();
   geo.levelCount = 1;
   geo.unique = true;

   ConvertVoxelDataToGeometry((VoxelGeometryVertex *)m.vertices.data(), (uint *)m.indices.data(), geo);
   return CreateVoxelNode(parent, geo);
}

RenderNodePtr RenderNode::CreateSharedCsgMeshNode(H3DNode parent, ResourceCacheKey const& key, CreateMeshLodLevelFn create_mesh_fn)
{   
   GeometryInfo geo;
   if (!Pipeline::GetInstance().GetSharedGeometry(key, geo)) {

      RN_LOG(7) << "creating new geometry for " << key.GetDescription();

      csg::mesh_tools::mesh m;
      for (int i = 0; i < MAX_LOD_LEVELS; i++) {
         create_mesh_fn(m, i);
         geo.vertexIndices[i + 1] = m.vertices.size();
         geo.indexIndicies[i + 1] = m.indices.size();
      }
      geo.levelCount = MAX_LOD_LEVELS;
      ConvertVoxelDataToGeometry((VoxelGeometryVertex *)m.vertices.data(), (uint *)m.indices.data(), geo);
      Pipeline::GetInstance().SetSharedGeometry(key, geo);
   }
   return CreateVoxelNode(parent, geo);
}

RenderNode::RenderNode()
{
}

RenderNode::RenderNode(H3DNode node) :
   _node(node)
{
   RN_LOG(9) << "attaching RenderNode to " << _node;
   ownedNodes.insert(node);
}

RenderNode::RenderNode(H3DNode node, H3DNode mesh, SharedGeometry geo, SharedMaterial mat) :
   _node(node),
   _meshNode(mesh),
   _geometry(geo),
   _material(mat)
{
   RN_LOG(9) << "attaching RenderNode to " << _node;
   ownedNodes.insert(node);
}

RenderNode::~RenderNode()
{
   DestroyHordeNode();
}

RenderNodePtr RenderNode::SetUserFlags(int flags)
{
   // why are flags on the mesh and not the model?
   h3dSetNodeParamI(_meshNode, H3DNodeParams::UserFlags, flags);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetName(const char *name)
{
   h3dSetNodeParamStr(_node, H3DNodeParams::NameStr, name);
   if (_meshNode) {
      h3dSetNodeParamStr(_meshNode, H3DNodeParams::NameStr, BUILD_STRING(name << " mesh").c_str());
   }
   return shared_from_this();
}

RenderNodePtr RenderNode::SetVisible(bool visible)
{
   h3dTwiddleNodeFlags(_node, H3DNodeFlags::NoDraw, !visible, false);
   SetCanQuery(visible);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetCanQuery(bool canQuery)
{
   h3dTwiddleNodeFlags(_node, H3DNodeFlags::NoRayQuery, !canQuery, false);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetGeometry(SharedGeometry geo)
{
   _geometry = geo;
   return shared_from_this();
}

RenderNodePtr RenderNode::SetMaterial(std::string const& material)
{
   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial(material);
   return SetMaterial(mat);
}

RenderNodePtr RenderNode::SetMaterial(SharedMaterial material)
{
   _material = material;
   ApplyMaterial();
   return shared_from_this();
}

RenderNodePtr RenderNode::SetPosition(csg::Point3f const& pos)
{
   csg::Point3f _, rot, scale;
   h3dGetNodeTransform(_node, &_.x, &_.y, &_.z, &rot.x, &rot.y, &rot.z, &scale.x, &scale.y, &scale.z);
   SetTransform(pos, rot, scale);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetRotation(csg::Point3f const& rot)
{
   csg::Point3f pos, _, scale;
   h3dGetNodeTransform(_node, &pos.x, &pos.y, &pos.z, &_.x, &_.y, &_.z, &scale.x, &scale.y, &scale.z);
   SetTransform(pos, rot, scale);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetScale(csg::Point3f const& scale)
{
   csg::Point3f pos, rot, _;
   h3dGetNodeTransform(_node, &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z, &_.x, &_.y, &_.z);
   SetTransform(pos, rot, scale);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetTransform(csg::Point3f const& pos, csg::Point3f const& rot, csg::Point3f const& scale)
{
   h3dSetNodeTransform(_node, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, scale.x, scale.y, scale.z);
   return shared_from_this();
}

RenderNodePtr RenderNode::AddChild(RenderNodePtr r)
{
   _children[r->GetNode()] = r;
   return shared_from_this();
}

void RenderNode::Initialize()
{
   ASSERT(!_unparentedRenderNode);
   _unparentedRenderNode = h3dAddGroupNode(1, "unparented render nodes");
   h3dSetNodeTransform(_unparentedRenderNode, -100000, -100000, -100000, 0, 0, 0, 1, 1, 1);
}

void RenderNode::Shutdown()
{
   if (_unparentedRenderNode) {
      h3dRemoveNode(_unparentedRenderNode);
      _unparentedRenderNode = 0;
   }
}

void RenderNode::Destroy()
{
   RN_LOG(7) << "RenderNode got Destroy on " << _node;
   DestroyHordeNode();

   _geometry.reset();
   _material.reset();
}

void RenderNode::DestroyHordeNode()
{
   if (_node) {
      // the node's about to go away.  destroying the node will destroy all it's
      // children!  that's not what we want.  reparent all our children under some
      // "unowned" node 
      int i = 0;
      if (_unparentedRenderNode) {
         while (true) {
            H3DNode child = h3dGetNodeChild(_node, i++);
            if (child == 0) {
               break;
            }
            // If this node is owned by some render node somewhere out there, move
            // it over to then unparented node
            if (ownedNodes.find(child) != ownedNodes.end()) {
               h3dSetNodeParent(child, _unparentedRenderNode);
            }
         }
      }
      _children.clear();
      RN_LOG(9) << "releasing RenderNode on " << _node;
      h3dRemoveNode(_node);
      ownedNodes.erase(_node);
      _node = 0;
   }
}

RenderNodePtr RenderNode::SetOverrideMaterial(SharedMaterial overrideMaterial)
{
   _overrideMaterial = overrideMaterial;
   ApplyMaterial();
   return shared_from_this();
}

void RenderNode::ApplyMaterial()
{
   if (_meshNode) {
      H3DRes mat = _material.get();
      if (_overrideMaterial.get()) {
         mat = _overrideMaterial.get();
      }
      h3dSetNodeParamI(_meshNode, H3DVoxelMeshNodeParams::MatResI, mat);
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
      --vi; --ni;    // covert 1 based indices to 0 based.

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
   
   std::string geoName = BUILD_STRING("geo" << nextId++);
   geo.vertexIndices[1] = vertices.size() / 3;
   geo.indexIndicies[1] = indices.size();
   geo.levelCount = 1;
   geo.geo = h3dutCreateGeometryRes(geoName.c_str(), vertices.size() / 3, indices.size(), vertices.data(),
                                    indices.data(), normalData.data(), nullptr, nullptr, nullptr, nullptr);
}
