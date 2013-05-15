#ifndef _RADIANT_CLIENT_PIPELINE_H
#define _RADIANT_CLIENT_PIPELINE_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "csg/point.h"
#include "core/singleton.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class QubicleMatrix;

struct Vertex {
   csg::Point3f         pos;
   csg::Point3f         normal;
   math3d::color3       color;
};

class Pipeline : public core::Singleton<Pipeline> {
   public:
      Pipeline();
      ~Pipeline();

      struct GeometryResource {
         H3DNode     geometry;
         int         numIndices;
         int         numVertices;
      };

      struct Geometry {
         std::vector<Vertex> vertices;
         std::vector<radiant::uint32> indices;
      };

      typedef std::unordered_map<std::string, H3DNode> NamedNodeMap;

      H3DNode GetTileEntity(const om::GridPtr grid, om::GridTilePtr tile, H3DRes parentNode);
      NamedNodeMap LoadQubicleFile(std::string const& uri);

      GeometryResource CreateMesh(std::string name, const Geometry& geo);
      Geometry OptimizeQubicle(const QubicleMatrix& m, const csg::Point3f& origin);

   private:
      H3DNode     orphaned_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
