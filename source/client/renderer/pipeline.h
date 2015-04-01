#ifndef _RADIANT_CLIENT_PIPELINE_H
#define _RADIANT_CLIENT_PIPELINE_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "csg/point.h"
#include "csg/meshtools.h"
#include "core/singleton.h"
#include "h3d_resource_types.h"
#include "resource_cache_key.h"
#include "render_node.h"
#include "core/static_string.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

enum UserFlags {
   None = 0,
   Terrain = 1
};

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      bool GetSharedGeometry(ResourceCacheKey const& key, GeometryInfo& geo);
      void SetSharedGeometry(ResourceCacheKey const& key, GeometryInfo const& geo);

      // dynamic meshes are likely unique (and therefore do not need to share geometry with anyone) and are likely
      // to change (e.g. the terrain).
      SharedMaterial GetSharedMaterial(std::string const& uri);

      RenderNodePtr CreateDesignationNode(H3DNode parent, csg::Region2f const& model, csg::Color4 const& outline_color, csg::Color4 const& stripes_color, int useCoarseCollisionBox=1);
      RenderNodePtr CreateStockpileNode(H3DNode parent, csg::Region2f const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      RenderNodePtr CreateSelectionNode(H3DNode parent, csg::Region2f const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      RenderNodePtr CreateRegionOutlineNode(H3DNode parent, csg::Region3f const& region, csg::Color4 const& edge_color, csg::Color4 const& face_color, std::string const& material);
      RenderNodePtr CreateRenderNodeFromMesh(H3DNode parent, csg::Mesh const& m);

      H3DRes CreateVoxelGeometryFromRegion(std::string const& geoName, csg::Region3 const& region);

      typedef std::function<void(csg::Mesh &, int lodLevel)> CreateMeshLodLevelFn;
      void CreateSharedGeometryFromGenerator(GeometryInfo& geo, ResourceCacheKey const& key, CreateMeshLodLevelFn const& create_mesh_fn, bool noInstancing=false);
      void CreateSharedGeometryFromOBJ(GeometryInfo& geo, ResourceCacheKey const& key, std::istream& is, bool noInstancing=false);
      void CreateSharedGeometryFromMesh(GeometryInfo& geo, ResourceCacheKey const& key, csg::Mesh const& m, bool noInstancing=false);
      void CreateGeometryFromMesh(GeometryInfo& geo, csg::Mesh const& m);

      void ConvertVoxelDataToGeometry(VoxelGeometryVertex *vertices, uint *indices, GeometryInfo& geo);

      csg::Mesh CreateMeshFromRegion(csg::Region3 const& region);

      H3DNode CreateVoxelMeshNode(H3DNode parent, GeometryInfo const& geo);
      H3DNode CreateVoxelMeshNode(H3DNode parent, GeometryInfo const& geo, SharedMaterial material);

   private:
      void AddDesignationBorder(csg::Mesh& m, csg::EdgeMap2f& edgemap);
      void AddDesignationStripes(csg::Mesh& m, csg::Region2f const& panels);
      RenderNodePtr CreateXZBoxNode(H3DNode parent, csg::Region2f const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color, float border_size);
      void CreateXZBoxNodeGeometry(csg::Mesh& mesh, csg::Region2f const& region, csg::Color4 const& interior_color, csg::Color4 const& border_color, float border_size);
      SharedGeometry CreateMeshGeometryFromObj(std::string const& geoName, std::istream& stream);
      void ConvertObjFileToGeometry(std::istream& stream, GeometryInfo &geo);

   private:
      typedef std::unordered_map<ResourceCacheKey, GeometryInfo, ResourceCacheKey::Hash> GeometryMap;

   private:
      int             unique_id_;
      GeometryMap     geometry_cache_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
