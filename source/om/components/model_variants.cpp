#include "pch.h"
#include "render_info.ridl.h"
#include "model_layer.ridl.h"
#include "model_variants.ridl.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, ModelVariants const& o)
{
   return (os << "[ModelVariants]");
}

void ModelVariants::ExtendObject(json::Node const& obj)
{
   // All entities with models must have a render_info...
   GetEntity().AddComponent<RenderInfo>();

   for (auto const& e : obj) {
      ModelLayerPtr layer = AddVariant(e.name());
      layer->Init(e);
   }
}

ModelLayerPtr ModelVariants::AddVariant(std::string const& name)
{
   ModelLayerPtr layer;
   if (variants_.Contains(name)) {
      layer = variants_.Get(name, nullptr);
   } else {
      layer = GetStore().AllocObject<ModelLayer>();
      variants_.Add(name, layer);
   }
   return layer;
}
