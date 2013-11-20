#include "pch.h"
#include "render_info.ridl.h"
#include "model_layer.ridl.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::om;

#if 0
std::ostream& om::operator<<(std::ostream& os, const ModelLayer& o)
{
   os << "[ModelLayer " << o.GetObjectId() << " variants:" << o.GetVariants() << "]";
   return os;
}
#endif

static std::unordered_map<std::string, ModelLayer::Layer> __str_to_layer; // xxx -- would LOVE initializer here..

void ModelLayer::Init(json::Node const& obj)
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
