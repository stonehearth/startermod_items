#include "pch.h"
#include <unordered_set>
#include "resources/res_manager.h"
#include "radiant_macros.h"
#include "radiant_stdutil.h"
#include "render_node.h"
#include "Horde3DUtils.h"
#include "pipeline.h"
#include "geometry_info.h"

using namespace ::radiant;
using namespace ::radiant::client;

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

   ASSERT(geo.type == GeometryInfo::HORDE_GEOMETRY);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/voxel.material.json");
   H3DNode node = h3dAddModelNode(parent, modelName.c_str(), geo.geo.get());

   int indexCount = geo.indexIndicies[geo.levelCount];
   int vertexCount = geo.vertexIndices[geo.levelCount];
   H3DNode meshNode = h3dAddMeshNode(node, meshName.c_str(), mat.get(), 0, indexCount, 0, vertexCount - 1);

   return std::make_shared<RenderNode>(node, meshNode, geo.geo, mat);
}

RenderNodePtr RenderNode::CreateVoxelModelNode(H3DNode parent, GeometryInfo const& geo)
{
   std::string modelName = BUILD_STRING("model" << nextId++);
   std::string meshName = BUILD_STRING("mesh" << nextId++);

   ASSERT(geo.type == GeometryInfo::VOXEL_GEOMETRY);

   SharedMaterial mat = Pipeline::GetInstance().GetSharedMaterial("materials/voxel.material.json");
   H3DNode modelNode = h3dAddVoxelModelNode(parent, meshName.c_str());
   H3DNode meshNode = h3dAddVoxelMeshNode(modelNode, modelName.c_str(), mat.get(), geo.geo.get());
   if (geo.noInstancing) {
      h3dSetNodeParamI(meshNode, H3DVoxelMeshNodeParams::NoInstancingI, true);
   }
   return std::make_shared<RenderNode>(modelNode, meshNode, geo.geo, mat);
}

RenderNode::RenderNode() :
   _node(0),
   _meshNode(0)
{
}

RenderNode::RenderNode(H3DNode node) :
   _node(node),
   _meshNode(0)
{
   RN_LOG(9) << "attaching RenderNode to " << _node;
   ownedNodes.insert(node);
}

RenderNode::RenderNode(H3DNode modelNode, H3DNode mesh, SharedGeometry geo, SharedMaterial mat) :
   _node(modelNode),
   _meshNode(mesh),
   _geometry(geo),
   _material(mat)
{
   RN_LOG(9) << "attaching RenderNode to " << _node;
   ownedNodes.insert(modelNode);
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

RenderNodePtr RenderNode::SetMaterial(core::StaticString material)
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
   float rx, ry, rz, sx, sy, sz;

   h3dGetNodeTransform(_node, NULL, NULL, NULL, &rx, &ry, &rz, &sx, &sy, &sz);
   h3dSetNodeTransform(_node, (float)pos.x, (float)pos.y, (float)pos.z, rx, ry, rz, sx, sy, sz);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetRotation(csg::Point3f const& rot)
{
   float x, y, z, sx, sy, sz;

   h3dGetNodeTransform(_node, &x, &y, &z, NULL, NULL, NULL, &sx, &sy, &sz);
   h3dSetNodeTransform(_node, x, y, z, (float)rot.x, (float)rot.y, (float)rot.z, sx, sy, sz);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetScale(csg::Point3f const& scale)
{
   float x, y, z, rx, ry, rz;

   h3dGetNodeTransform(_node, &x, &y, &z, &rx, &ry, &rz, NULL, NULL, NULL);
   h3dSetNodeTransform(_node, x, y, z, rx, ry, rz, (float)scale.x, (float)scale.y, (float)scale.z);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetTransform(csg::Point3f const& pos, csg::Point3f const& rot, csg::Point3f const& scale)
{
   h3dSetNodeTransform(_node, (float)pos.x, (float)pos.y, (float)pos.z,
                              (float)rot.x, (float)rot.y, (float)rot.z,
                              (float)scale.x, (float)scale.y, (float)scale.z);
   return shared_from_this();
}

RenderNodePtr RenderNode::SetPolygonOffset(float x, float y)
{
   h3dSetNodeParamI(_node, H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(_node, H3DModel::PolygonOffsetF, 0, x);
   h3dSetNodeParamF(_node, H3DModel::PolygonOffsetF, 1, y);

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
   _unparentedRenderNode = h3dAddGroupNode(h3dGetRootNode(0), "unparented render nodes");
   h3dSetNodeTransform(_unparentedRenderNode, 0, 0, 0, 0, 0, 0, 1, 1, 1);
   h3dSetNodeFlags(_unparentedRenderNode, H3DNodeFlags::Inactive | H3DNodeFlags::NoCull, true);
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
