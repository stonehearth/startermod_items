#include "pch.h"
#include "radiant_macros.h"
#include "render_node.h"
#include "Horde3DUtils.h"

using namespace ::radiant;
using namespace ::radiant::client;

static int nextId = 1;

RenderNode::RenderNode() :
   _id(nextId++)
{
}

RenderNode::RenderNode(H3DNode node) :
   _node(node),
   _id(nextId++)
{
}

RenderNode::~RenderNode()
{
}

RenderNode& RenderNode::SetGeometry(SharedGeometry geo)
{
   _geometry = geo;
   return *this;
}

RenderNode& RenderNode::SetMaterial(std::string const& material)
{
   H3DRes mat = h3dAddResource(H3DResTypes::Material, material.c_str(), 0);
   return SetMaterial(mat);
}

RenderNode& RenderNode::SetMaterial(SharedMaterial material)
{
   _material = material;
   ApplyMaterial();
   return *this;
}

RenderNode& RenderNode::SetMesh(SharedNode meshNode)
{
   _meshNode = meshNode;
   ApplyMaterial();
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
