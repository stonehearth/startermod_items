#include "radiant.h"
#include "mob.ridl.h"
#include "render_info.ridl.h"
#include "entity_container.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, RenderInfo const& o)
{
   return (os << "[RenderInfo]");
}

void RenderInfo::ConstructObject()
{
   Component::ConstructObject();
   scale_ = 0.1f;
   visible_ = true;
   cache_model_geometry_ = true;
}

void RenderInfo::LoadFromJson(json::Node const& obj)
{
   scale_ = obj.get<float>("scale", *scale_);
   material_ = obj.get<std::string>("material", "");
   model_variant_ = obj.get<std::string>("model_variant", *model_variant_);
   animation_table_ = obj.get<std::string>("animation_table", *animation_table_);
   cache_model_geometry_ = obj.get<bool>("cache_model_geometry", true);

   color_map_ = obj.get<std::string>("color_map", *color_map_);
   json::Node materialMaps = obj.get<json::Node>("material_maps", json::Node());
   material_maps_.Clear();
   for (json::Node const& entry : materialMaps) {
      material_maps_.Add(entry.as<std::string>());
   }
}

void RenderInfo::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("scale", GetScale());
   node.set("material", GetMaterial());
   node.set("model_variant", GetModelVariant());
   node.set("animation_table", GetAnimationTable());
   node.set("colorindex", GetColorMap());
   node.set("cache_model_geometry", GetCacheModelGeometry());

   JSONNode materialMaps(JSON_ARRAY);
   for (std::string const& map : EachMaterialMap()) {
      materialMaps.push_back(JSONNode("", map));
   }
   node.set("material_maps", materialMaps);
}

void RenderInfo::AttachEntity(om::EntityRef e)
{
   auto entity = e.lock();
   if (entity) {
      RemoveFromWorld(entity);
      attached_entities_.Add(e);
   }
}

EntityRef RenderInfo::RemoveEntity(std::string const& uri)
{
   for (auto const& e : attached_entities_) {
      auto entity = e.lock();
      if (entity && entity->GetUri() == uri) {
         attached_entities_.Remove(e);
         return e;
      }
   }
   return EntityRef();
}

void RenderInfo::RemoveFromWorld(EntityPtr entity)
{
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      auto parent = mob->GetParent().lock();
      if (parent) {
         parent->GetComponent<EntityContainer>()->RemoveChild(entity->GetObjectId());
      }
      ASSERT(mob->GetParent().expired());
   }
}
