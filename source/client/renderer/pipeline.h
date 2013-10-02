#ifndef _RADIANT_CLIENT_PIPELINE_H
#define _RADIANT_CLIENT_PIPELINE_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "csg/point.h"
#include "csg/meshtools.h"
#include "core/singleton.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class QubicleMatrix;

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      typedef std::unordered_map<std::string, H3DNodeUnique> NamedNodeMap;

      NamedNodeMap LoadQubicleFile(std::string const& uri);
      H3DNodeUnique AddQubicleNode(H3DNode parent, const QubicleMatrix& m, const csg::Point3f& origin);
      H3DNodeUnique AddMeshNode(H3DNode parent, const csg::mesh_tools::mesh& m);

   private:
      H3DNodeUnique     orphaned_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
