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
   bool unique;
   SharedGeometry geo;

   GeometryInfo() : levelCount(0), geo(0), unique(false) {
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


class RenderNode : public  std::enable_shared_from_this<RenderNode> {
public:  
   typedef std::function<void(csg::mesh_tools::mesh &, int lodLevel)> CreateMeshLodLevelFn;

   static RenderNodePtr CreateGroupNode(H3DNode parent, std::string const& name);
   static RenderNodePtr CreateMeshNode(H3DNode parent, GeometryInfo const& geo);
   static RenderNodePtr CreateVoxelNode(H3DNode parent, GeometryInfo const& geo);

   static RenderNodePtr CreateObjNode(H3DNode parent, std::string const& uri);
   static RenderNodePtr CreateCsgMeshNode(H3DNode parent, csg::mesh_tools::mesh const& m);
   static RenderNodePtr CreateSharedCsgMeshNode(H3DNode parent, ResourceCacheKey const& key, CreateMeshLodLevelFn cb);

   static void Initialize();
   static void Shutdown();

   ~RenderNode();

   H3DNode GetNode() const { return _node; }

   RenderNodePtr SetName(const char* name);
   RenderNodePtr SetVisible(bool visible);
   RenderNodePtr SetCanQuery(bool canQuery);
   RenderNodePtr SetUserFlags(int flags);
   RenderNodePtr SetGeometry(SharedGeometry geo);
   RenderNodePtr SetMaterial(std::string const& material);
   RenderNodePtr SetMaterial(SharedMaterial mat);
   RenderNodePtr SetOverrideMaterial(SharedMaterial mat);
   RenderNodePtr SetPosition(csg::Point3f const& pos);
   RenderNodePtr SetRotation(csg::Point3f const& rot);
   RenderNodePtr SetScale(csg::Point3f const& scale);
   RenderNodePtr SetTransform(csg::Point3f const& pos, csg::Point3f const& rot, csg::Point3f const& scale);

   RenderNodePtr AddChild(RenderNodePtr r);

   void Destroy();

private:
   RenderNode();

public: // just because we need std::make_shared<>  UG!
   RenderNode(H3DNode node);
   RenderNode(H3DNode node, H3DNode mesh, SharedGeometry geo, SharedMaterial mat);

   void ApplyMaterial();
   void DestroyHordeNode();

   static H3DNode GetUnparentedNode() { return _unparentedRenderNode; }

private:
   static void ConvertVoxelDataToGeometry(VoxelGeometryVertex *vertices, uint *indices, GeometryInfo& geo);
   static void ConvertObjFileToGeometry(std::istream& stream, GeometryInfo& geo);
   static H3DNode _unparentedRenderNode;

private:
   std::unordered_map<H3DNode, RenderNodePtr> _children;
   H3DNode        _node;
   H3DNode        _meshNode; // The lifetime of the mesh is controlled by the _node, therefore it does not need to be auto-deleted
   SharedGeometry _geometry;
   SharedMaterial _material;
   SharedMaterial _overrideMaterial;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
