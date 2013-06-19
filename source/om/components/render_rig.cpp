#include "pch.h"
#include "render_rig.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderRig::InitializeRecordFields() 
{
   Component::InitializeRecordFields();
   AddRecordField("rigs", rigs_);
   AddRecordField("scale", scale_);
   AddRecordField("animation_table", animationTable_);
   scale_ = 0.1f;
}

void RenderRig::ExtendObject(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();

   scale_ = json::get<float>(node, "scale", *scale_);
   animationTable_ = json::get<std::string>(node, "animation_table", *animationTable_);

   for (auto const& e : node["models"]) {
      if (e.type() == JSON_STRING) {
         AddRig(e.as_string());
      } else if (e.type() == JSON_NODE) {
         if (e["type"].as_string() == "one_of") {
            JSONNode const& items = e["items"];
            uint c = rand() * items.size() / RAND_MAX;
            ASSERT(c < items.size());
            AddRig(items.at(c).as_string());
         }
      }
   }
}
