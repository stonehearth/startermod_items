#ifndef _RADIANT_CLIENT_PIPELINE_H
#define _RADIANT_CLIENT_PIPELINE_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "resources/model.h" // xxx -0 no!
#include "csg/point.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class QubicleMatrix;

struct Vertex {
   csg::Point3f         pos;
   csg::Point3f         normal;
   math3d::color3       color;
};

class Pipeline
{
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

      H3DRes GetActorEntity(const resources::Model &model);
      H3DNode GetTileEntity(const om::GridPtr grid, om::GridTilePtr tile, H3DRes parentNode);

      GeometryResource CreateMesh(std::string name, const Geometry& geo);
      Geometry OptimizeQubicle(const QubicleMatrix& m, const csg::Point3f& origin);

   protected:
      //void LoadActorMaterial(Ogre::Entity *entity, const resources::Model &model);
      //void AddDebugWindow(std::string texture);

   protected:
      //Ogre::MaterialPtr                         _actorMaterial;
      //std::map<std::string, Ogre::MaterialPtr>  _actorMaterials;
      //Ogre::MaterialPtr                         _tileMaterial;
      //Ogre::VertexData                          *_tileVertexData;
      //std::vector<TextureDebugWindow>           _debugWindows;
      //int                                       _nextEntityId;
      //std::map<string, Ogre::Entity*>           models_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
