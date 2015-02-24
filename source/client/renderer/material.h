#ifndef _RADIANT_CLIENT_RENDERER_MATERIAL_H
#define _RADIANT_CLIENT_RENDERER_MATERIAL_H

#include "namespace.h"
#include "Horde3D.h"
#include "lib/json/node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Material
{
private:
   struct SamplerDesc {
      std::string name;
      std::string path;
      int numFrames;
      float frameRate;
   };

   public:
      Material() {}
      void load(json::Node const& materialNode);
      virtual ~Material() {}

      std::string const& getName() const { return _name; }
      void applyToMaterialRes(H3DRes node);

   private:
      NO_COPY_CONSTRUCTOR(Material);

   private:
      std::string _name;
      std::vector<SamplerDesc> _samplers;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_MATERIAL_H