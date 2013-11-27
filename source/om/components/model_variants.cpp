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
      ModelLayerPtr layer = GetStore().AllocObject<ModelLayer>();
      layer->Init(e);
      model_variants_.Add(layer);
   }
}


ModelLayerPtr ModelVariants::GetModelVariant(std::string const& v) const
{
   ModelLayerPtr best;

   for (auto const& e : model_variants_) {
      std::string variants = e->GetVariants();

      // Fall back to the first entry at all...
      if (!best) {
         best = e;
      }
      // Always prefer the first entry which has no variants...
      if (v.empty()) {
         if (variants.empty()) {
            best = e;
            break;
         }
      } else {
         std::vector<std::string> tags;
         boost::split(tags, variants, boost::is_any_of(" "));
         if (stdutil::contains(tags, v)) {
            return e;
         }
      }
   }
   return best;
}
