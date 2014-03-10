#include "pch.h"
#include "render_info.ridl.h"
#include "model_layer.ridl.h"
#include "lib/json/node.h"
#include "csg/random_number_generator.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, const ModelLayer& o)
{
   os << "[ModelLayer " << o.GetObjectId() << "]";
   return os;
}

static std::unordered_map<std::string, ModelLayer::Layer> __str_to_layer; // xxx -- would LOVE initializer here..

void ModelLayer::ConstructObject()
{
   Record::ConstructObject();
   layer_ = Layer::SKELETON;
}

void ModelLayer::LoadFromJson(json::Node const& obj)
{
   csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();

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

   for (const auto& e : obj.get("models", json::Node())) {
      std::string model_name;
      if (e.type() == JSON_STRING) {
         model_name = e.as<std::string>();
      } else if (e.type() == JSON_NODE) {
         if (e.get<std::string>("type", "") == "one_of") {
            json::Node items = e.get("items", json::Node());
            int c = rng.GetInt(0, items.size() - 1);
            ASSERT(c < items.size());
            model_name = items.get<std::string>(c);
         }
      }
      models_.Add(model_name);
   }
}

void ModelLayer::SerializeToJson(json::Node& node) const
{
   Object::SerializeToJson(node);
   static const std::string layer_names[] = {
      "skeleton",
      "skin",
      "clothing",
      "armor",
      "cloak",
   };
   node.set("layer", layer_names[GetLayer()]);
   for (auto const& model : EachModel()) {
      JSONNode n("", model);
      node.add(json::Node(n));
   }
}
