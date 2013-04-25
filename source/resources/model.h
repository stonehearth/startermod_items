#ifndef _RADIANT_RESOURCES_MODEL_H
#define _RADIANT_RESOURCES_MODEL_H

#include "math3d.h"
#include "namespace.h"
#include "radiant.pb.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Model {
   public:
      Model();
      Model(std::string mesh, std::string texture, std::string material, const math3d::point3 &offset);

      void SetMesh(std::string mesh);
      void SetTexture(std::string texture);
      void SetMaterial(std::string material);
      void SetOffset(const math3d::point3 &offset);

      std::string GetMesh() const;
      std::string GetTexture() const;
      std::string GetMaterial() const;
      math3d::point3 GetOffset() const;

   protected:
      std::string         _mesh;
      std::string         _texture;
      std::string         _material;
      math3d::point3 _offset;
};


END_RADIANT_RESOURCES_NAMESPACE


#endif //  _RADIANT_RESOURCES_MODEL_H
 