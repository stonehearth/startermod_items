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
#include "lib/voxel/forward_defines.h"
#include "render_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

enum UserFlags {
   None = 0,
   Terrain = 1
};

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      voxel::QubicleFile* LoadQubicleFile(std::string const& name);

      bool GetSharedGeometry(ResourceCacheKey const& key, GeometryInfo& geo);
      void SetSharedGeometry(ResourceCacheKey const& key, GeometryInfo const& geo);

      // dynamic meshes are likely unique (and therefore do not need to share geometry with anyone) and are likely
      // to change (e.g. the terrain).
      SharedMaterial GetSharedMaterial(std::string const& uri);

      RenderNodePtr CreateDesignationNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& outline_color, csg::Color4 const& stripes_color);
      RenderNodePtr CreateStockpileNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      RenderNodePtr CreateSelectionNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      H3DRes CreateVoxelGeometryFromRegion(std::string const& geoName, csg::Region3 const& region);
      csg::mesh_tools::mesh CreateMeshFromRegion(csg::Region3 const& region);

   private:
      void AddDesignationBorder(csg::mesh_tools::mesh& m, csg::EdgeMap2& edgemap);
      void AddDesignationStripes(csg::mesh_tools::mesh& m, csg::Region2 const& panels);
      RenderNodePtr CreateXZBoxNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color, float border_size);
      void CreateXZBoxNodeGeometry(csg::mesh_tools::mesh& mesh, csg::Region2 const& region, csg::Color4 const& interior_color, csg::Color4 const& border_color, float border_size);
      SharedGeometry CreateMeshGeometryFromObj(std::string const& geoName, std::istream& stream);

   private:
      typedef std::unordered_map<std::string, voxel::QubicleFilePtr> QubicleMap;
      typedef std::unordered_map<ResourceCacheKey, GeometryInfo, ResourceCacheKey::Hash> GeometryMap;

   private:
      int             unique_id_;
      QubicleMap      qubicle_files_; // xxx: shouldn't this be in the resource manager?
      GeometryMap     geometry_cache_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
