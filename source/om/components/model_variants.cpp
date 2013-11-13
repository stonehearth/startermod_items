#include "pch.h"
#include "render_info.h"
#include "model_variants.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const ModelVariant& o)
{
   os << "[ModelVariant " << o.GetObjectId() << " variants:" << o.GetVariants() << "]";
   return os;
}


void ModelVariants::ExtendObject(json::Node const& obj)
{
   // All entities with models must have a render_info...
   GetEntity().AddComponent<RenderInfo>();
   for (auto const& e : obj) {
      ModelVariantPtr layer = GetStore().AllocObject<ModelVariant>();
      layer->ExtendObject(e);
      variants_.Insert(layer);
   }
}


ModelVariantPtr ModelVariants::GetModelVariant(std::string const& v) const
{
   ModelVariantPtr best;

   for (auto const& e : variants_) {
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

static std::unordered_map<std::string, ModelVariant::Layer> __str_to_layer; // xxx -- would LOVE initializer here..

void ModelVariant::ExtendObject(json::Node const& obj)
{
   if (__str_to_layer.empty()) {
      __str_to_layer["skeleton"] = Layer::SKELETON;
      __str_to_layer["skin"]  = Layer::SKIN;
      __str_to_layer["clothing"] = Layer::CLOTHING;
      __str_to_layer["armor"] = Layer::ARMOR;
      __str_to_layer["cloak"] = Layer::CLOAK;
   }

   std::string layer_type = obj.get<std::string>("layer", "");
   if (layer_type.empty()) {
      layer_type = "skin";
   }
   auto i = __str_to_layer.find(layer_type);
   layer_ = (i != __str_to_layer.end() ? i->second : SKIN);

   variants_ = obj.get<std::string>("variants", "");

   for (const auto& node : obj.get("models", JSONNode())) {
      json::Node e(node);
      std::string model_name;
      if (e.type() == JSON_STRING) {
         model_name = e.GetNode().as_string();
      } else if (e.type() == JSON_NODE) {
         if (e.get<std::string>("type", "") == "one_of") {
            JSONNode items = e.get("items", JSONNode());
            uint c = rand() * items.size() / RAND_MAX;
            ASSERT(c < items.size());
            model_name = items.at(c).as_string();
         }
      }
      models_.Insert(model_name);
   }
}
