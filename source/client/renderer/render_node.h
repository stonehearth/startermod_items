#ifndef _RADIANT_CLIENT_UNIQUE_RENDERABLE_H
#define _RADIANT_CLIENT_UNIQUE_RENDERABLE_H

#include "namespace.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include "csg/point.h"
#include "csg/meshtools.h"
#include "h3d_resource_types.h"
#include "resource_cache_key.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

#define MAX_LOD_LEVELS 4

struct GeometryInfo {
   int vertexIndices[MAX_LOD_LEVELS + 1];
   int indexIndicies[MAX_LOD_LEVELS + 1];
   int levelCount;
   SharedGeometry geo;

   GeometryInfo() : levelCount(0), geo(0) {
      memset(vertexIndices, 0, sizeof vertexIndices);
      memset(indexIndicies, 0, sizeof indexIndicies);
   }
};

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
   
   typedef std::function<void(csg::mesh_tools::mesh &, int lodLevel)> CreateMeshLodLevelFn;

   static RenderNode CreateMeshNode(H3DNode parent, GeometryInfo const& geo);
   static RenderNode CreateVoxelNode(H3DNode parent, GeometryInfo const& geo);

   static RenderNode CreateObjFileNode(H3DNode parent, std::string const& uri);
   static RenderNode CreateCsgMeshNode(H3DNode parent, csg::mesh_tools::mesh const& m);
   static RenderNode CreateSharedCsgMeshNode(H3DNode parent, ResourceCacheKey const& key, CreateMeshLodLevelFn cb);

   ~RenderNode();

   H3DNode GetNode() const { return _node.get(); }

   RenderNode& SetUserFlags(int flags);
   RenderNode& SetGeometry(SharedGeometry geo);
   RenderNode& SetMaterial(std::string const& material);
   RenderNode& SetMaterial(SharedMaterial mat);
   RenderNode& SetOverrideMaterial(SharedMaterial mat);
   RenderNode& SetTransform(csg::Point3f const& pos, csg::Point3f const& rot, csg::Point3f const& scale);

   RenderNode& AddChild(const RenderNode& r);

   void Destroy();

private:
   static void ConvertVoxelDataToGeometry(VoxelGeometryVertex *vertices, uint *indices, GeometryInfo& geo);
   static void ConvertObjFileToGeometry(std::istream& stream, GeometryInfo& geo);

   RenderNode(H3DNode node, H3DNode mesh, SharedGeometry geo, SharedMaterial mat);
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
