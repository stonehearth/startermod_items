#ifndef _RADIANT_CLIENT_UNIQUE_RENDERABLE_H
#define _RADIANT_CLIENT_UNIQUE_RENDERABLE_H

#include "namespace.h"
#include "Horde3D.h"
#include "h3d_resource_types.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

/*
 * class RenderNode
 *
 * Encapsulates the stateful side of the Horde scene graph and object lifetime.
 * For example, we occasionally want to override the material on a node.  The
 * RenderNode facilitates this by coupling the mode and mesh nodes in one object
 * and exposing SetMaterial() and SetOverrideMaterial() methods
 */

class RenderNode {
public:
   RenderNode();
   RenderNode(H3DNode node);
   ~RenderNode();

   H3DNode GetNode() const { return _node.get(); }

   RenderNode& SetGeometry(SharedGeometry geo);
   RenderNode& SetMaterial(std::string const& material);
   RenderNode& SetMaterial(SharedMaterial mat);
   RenderNode& SetOverrideMaterial(SharedMaterial mat);
   RenderNode& SetMesh(SharedNode mesh);
   RenderNode& AddChild(const RenderNode& r);

   void Destroy();

private:
   void ApplyMaterial();

private:
   std::vector<RenderNode> _children;
   int            _id;
   SharedNode     _node;
   SharedNode     _meshNode;
   SharedGeometry _geometry;
   SharedMaterial _material;
   SharedMaterial _overrideMaterial;
};


END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
