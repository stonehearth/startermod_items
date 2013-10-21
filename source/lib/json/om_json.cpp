#include "radiant.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "om/object_formatter/object_formatter.h"
#include "lib/json/om_json.h"

using namespace radiant;
using namespace radiant::json;


#define OM_OBJECT(Cls, lower) \
   template <> json::Node json::encode(om::Cls const& obj) { \
      JSONNode result; \
      result.push_back(JSONNode("error", BUILD_STRING("someone needs to implement " << #Cls << "::ToJson in the object formatter!"))); \
      return result; \
   }

      OM_OBJECT(Effect,         effect)
      OM_OBJECT(Sensor,         sensor)
      OM_OBJECT(Aura,           aura)
      OM_OBJECT(TargetTable,        target_table)
      OM_OBJECT(TargetTableGroup,   target_table_group)
      OM_OBJECT(TargetTableEntry,   target_table_entry)
      OM_OBJECT(Clock,                 clock)
      OM_OBJECT(ModelVariant,          model_variant)
      OM_OBJECT(ModelVariants,         model_variants)
      OM_OBJECT(SphereCollisionShape,  sphere_collision_shape)
      OM_OBJECT(RegionCollisionShape,  region_collision_shape)
      OM_OBJECT(Terrain,               terrain)
      OM_OBJECT(VerticalPathingRegion, vertical_pathing_region)
      OM_OBJECT(EffectList,            effect_list)
      OM_OBJECT(RenderInfo,            render_info)
      OM_OBJECT(SensorList,            sensor_list)
      OM_OBJECT(Attributes,            attributes)
      OM_OBJECT(AuraList,              aura_list)
      OM_OBJECT(TargetTables,          target_tables)
      OM_OBJECT(Destination,           destination)
      OM_OBJECT(RenderRegion,          render_region)
      OM_OBJECT(Paperdoll,             paperdoll)
      OM_OBJECT(Item,                  item)
      OM_OBJECT(CarryBlock,            carry_block)

#undef OM_OBJECT

template <> Node json::encode(om::LuaComponents const& obj)
{
   om::ObjectFormatter f;
   Node node;

   for (auto const& entry : obj.GetComponentMap()) {
      node.set(entry.first, f.GetPathToObject(entry.second));
   }
   return node;
}

template <> Node json::encode(om::Entity const& obj)
{
   om::ObjectFormatter f;
   std::string debug_text = obj.GetDebugText();
   std::string uri = obj.GetUri();
   Node node;

   if (!uri.empty()) {
      node.set("uri", uri);
   }
   if (!debug_text.empty()) {
      node.set("debug_text", debug_text);
   }
   auto const& components = obj.GetComponents();
   for (auto const& entry : components) {
      dm::ObjectPtr c = entry.second;
      if (c->GetObjectType() == om::LuaComponentsObjectType) {
         JSONNode luac = json::encode(static_cast<om::LuaComponents&>(*c.get()));
         for (auto const& entry : luac) {
            node.set(entry.name(), entry);
         }
      } else {
         node.set(om::GetObjectNameLower(c), f.GetPathToObject(c));
      }
   }
   return node;
}

template <> Node json::encode(om::EntityContainer const& obj)
{
   om::ObjectFormatter f;
   Node node;

   for (auto const& entry : obj.GetChildren()) {
      om::EntityPtr child = entry.second.lock();
      if (child) {
         node.set(stdutil::ToString(entry.first), f.GetPathToObject(child));
      }
   }
   return node;
}

template <> Node json::encode(om::UnitInfo const& obj)
{
   Node node;
   
   node.set("name", obj.GetDisplayName());
   node.set("description", obj.GetDescription());
   node.set("faction", obj.GetFaction());
   node.set("icon", obj.GetIcon());

   return node;
}

template <> Node json::encode(om::Mob const& obj)
{
   Node node;

   node.set("transform", obj.GetTransform());
   node.set("entity", om::ObjectFormatter().GetPathToObject(obj.GetEntityPtr()));
   node.set("moving", obj.GetMoving());
   om::MobPtr parent = obj.GetParent();
   if (parent) {
      node.set("parent", om::ObjectFormatter().GetPathToObject(parent));
   }

   return node;
}

template <> Node json::encode(om::DataBinding const& obj)
{
   return obj.GetJsonData();
}

template <> Node json::encode(om::DataBindingP const& obj)
{
   return obj.GetJsonData();
}

template <> Node json::encode(om::JsonStore const& obj)
{
   return Node(obj.Get());
}

template <> Node json::encode(om::ErrorBrowser const& obj)
{
   om::ObjectFormatter f;
   Node node;

   JSONNode entries(JSON_ARRAY);
   entries.set_name("entries");
   for (auto const& e: obj.GetEntries()) {
      entries.push_back(JSONNode("", f.GetPathToObject(e)));
   }

   node.set("entries", entries);

   return node;
}
