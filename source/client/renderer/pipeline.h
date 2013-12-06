#ifndef _RADIANT_CLIENT_PIPELINE_H
#define _RADIANT_CLIENT_PIPELINE_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "csg/point.h"
#include "csg/meshtools.h"
#include "core/singleton.h"
#include "lib/voxel/forward_defines.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      voxel::QubicleFile* LoadQubicleFile(std::string const& name);
      H3DNodeUnique AddQubicleNode(H3DNode parent, const voxel::QubicleMatrix& m, const csg::Point3f& origin, H3DNode *mesh = nullptr);
      H3DNodeUnique AddMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m, H3DNode *mesh = nullptr);
      H3DNodeUnique CreateBlueprintNode(H3DNode parent, csg::Region3 const& model, float thickness, std::string const& material_path);
      H3DNodeUnique CreateVoxelNode(H3DNode parent, csg::Region3 const& model, std::string const& material_path);
      H3DNodeUnique CreateDesignationNode(H3DNode parent, csg::Region2 const& model, csg::Color3 const& outline_color, csg::Color3 const& stripes_color);
      H3DNodeUnique CreateQubicleMatrixNode(H3DNode parent, std::string const& qubicle_file, std::string const& qubicle_matrix, csg::Point3f const& origin);

   private:
      void AddDesignationBorder(csg::mesh_tools::mesh& m, csg::EdgeMap2& edgemap);
      void AddDesignationStripes(csg::mesh_tools::mesh& m, csg::Region2 const& panels);
      H3DNode CreateModel(H3DNode parent, csg::mesh_tools::mesh const& mesh, std::string const& material_path);
      H3DRes CreateGeometryFromQubicleMatrix(const std::string& geoName, const voxel::QubicleMatrix& m, const csg::Point3f& origin);
   private:
      H3DNodeUnique     orphaned_;
      std::unordered_map<std::string, voxel::QubicleFilePtr>   qubicle_files_;
      int               unique_id_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
