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

BEGIN_RADIANT_CLIENT_NAMESPACE

enum UserFlags {
   None = 0,
   Terrain = 1
};

class UniqueRenderable {
   public:
      UniqueRenderable(H3DNode node, H3DRes geometryResource);
      UniqueRenderable();

      void AddChild(const UniqueRenderable& r);
      void Destroy();
      H3DNode getNode() const { return _node; }

      friend class Pipeline;
   private:
      std::vector<UniqueRenderable> _children;
      H3DNode _node;
      H3DRes _geoRes;
};

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      voxel::QubicleFile* LoadQubicleFile(std::string const& name);

      // dynamic meshes are likely unique (and therefore do not need to share geometry with anyone) and are likely
      // to change (e.g. the terrain).
      UniqueRenderable AddDynamicMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m, std::string const& material, int userFlags);
      H3DNode AddSharedMeshNode(H3DNode parent, ResourceCacheKey const& key, std::string const& material, std::function<void(csg::mesh_tools::mesh &, int)> create_mesh_fn);

      UniqueRenderable CreateVoxelNode(H3DNode parent, csg::Region3 const& model, std::string const& material_path, csg::Point3f const& offset, int userFlags);
      UniqueRenderable CreateBlueprintNode(H3DNode parent, csg::Region3 const& model, float thickness, std::string const& material_path, csg::Point3f const& offset);
      UniqueRenderable CreateDesignationNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& outline_color, csg::Color4 const& stripes_color);
      UniqueRenderable CreateStockpileNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      UniqueRenderable CreateSelectionNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color);
      H3DNodeUnique CreateQubicleMatrixNode(H3DNode parent, std::string const& qubicle_file, std::string const& qubicle_matrix, csg::Point3f const& origin);
      H3DRes CreateVoxelGeometryFromRegion(std::string const& geoName, csg::Region3 const& region);
      csg::mesh_tools::mesh CreateMeshFromRegion(csg::Region3 const& region);

   private:
      void AddDesignationBorder(csg::mesh_tools::mesh& m, csg::EdgeMap2& edgemap);
      void AddDesignationStripes(csg::mesh_tools::mesh& m, csg::Region2 const& panels);
      UniqueRenderable CreateXZBoxNode(H3DNode parent, csg::Region2 const& model, csg::Color4 const& interior_color, csg::Color4 const& border_color, double border_size);
      void CreateXZBoxNodeGeometry(csg::mesh_tools::mesh& mesh, csg::Region2 const& region, csg::Color4 const& interior_color, csg::Color4 const& border_color, double border_size);      
      H3DNode CreateModelNode(H3DNode parent, H3DRes geometry, std::string const& material, int flag);
      H3DRes ConvertMeshToGeometryResource(const csg::mesh_tools::mesh& m, int indexOffsets[], int vertexOffsets[], int numLodLevels);

   private:
      typedef std::unordered_map<std::string, voxel::QubicleFilePtr> QubicleMap;
      typedef std::unordered_map<ResourceCacheKey, H3DRes, ResourceCacheKey::Hash> ResourceMap;

   private:
      int             unique_id_;
      QubicleMap      qubicle_files_; // xxx: shouldn't this be in the resource manager?
      ResourceMap     resource_cache_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
