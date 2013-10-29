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

      typedef std::unordered_map<std::string, H3DNodeUnique> NamedNodeMap;

      NamedNodeMap LoadQubicleFile(std::string const& uri);
      H3DNodeUnique AddQubicleNode(H3DNode parent, const voxel::QubicleMatrix& m, const csg::Point3f& origin, H3DNode *mesh = nullptr);
      H3DNodeUnique AddMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m, H3DNode *mesh = nullptr);
      H3DNodeUnique CreateBlueprintNode(H3DNode parent, csg::Region3 const& model, float thickness, std::string const& material_path);
      H3DNodeUnique CreateVoxelNode(H3DNode parent, csg::Region3 const& model, std::string const& material_path);

   private:
      H3DNode CreateModel(H3DNode parent, csg::mesh_tools::mesh const& mesh, std::string const& material_path);

   private:
      H3DNodeUnique     orphaned_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
