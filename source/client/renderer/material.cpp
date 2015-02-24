#include "pch.h"
#include "material.h"

using namespace ::radiant;
using namespace ::radiant::client;


void Material::load(json::Node const& materialNode)
{
   _name = materialNode.get("name", "");

   for (auto const& sampler : materialNode.get_node("samplers")) {
      Material::SamplerDesc sd;
      sd.name = sampler.get("name", "");
      sd.path = sampler.get("map", "");
      sd.numFrames = sampler.get("numAnimationFrames", 0);
      sd.frameRate = sampler.get("frameRate", 24.0f);

      _samplers.push_back(sd);
   }
}

void Material::applyToMaterialRes(H3DRes node)
{
   for (auto const& sampler : _samplers) {
      h3dSetMaterialSampler(node, sampler.name.c_str(), sampler.path.c_str(), sampler.numFrames, sampler.frameRate);
   }
}
