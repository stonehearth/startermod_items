#include "radiant.h"
#include "radiant_stdutil.h"
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

      OM_OBJECT(Sensor,         sensor)
      OM_OBJECT(TargetTable,        target_table)
      OM_OBJECT(TargetTableGroup,   target_table_group)
      OM_OBJECT(TargetTableEntry,   target_table_entry)
      OM_OBJECT(Clock,                 clock)
      OM_OBJECT(ModelLayer,            model_layer)
      OM_OBJECT(ModelVariants,         model_variants)
      OM_OBJECT(Terrain,               terrain)
      OM_OBJECT(VerticalPathingRegion, vertical_pathing_region)
      OM_OBJECT(RenderInfo,            render_info)
      OM_OBJECT(SensorList,            sensor_list)
      OM_OBJECT(TargetTables,          target_tables)
      OM_OBJECT(Item,                  item)

#undef OM_OBJECT

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
      node.set(entry.first, f.GetPathToObject(c));
   }
   return node;
}

template <> Node json::encode(om::EntityContainer const& obj)
{
   om::ObjectFormatter f;
   Node node;

   for (auto const& entry : obj.EachChild()) {
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
   node.set("character_sheet_info", obj.GetCharacterSheetInfo());
   node.set("portrait", obj.GetPortrait());
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
   om::EntityPtr parent = obj.GetParent().lock();
   if (parent) {
      node.set("parent", om::ObjectFormatter().GetPathToObject(parent));
   }

   return node;
}

template <> Node json::encode(om::RegionCollisionShape const& obj)
{
   Node node;

   om::Region3BoxedPtr region = obj.GetRegion();
   if (region) {
      node.set("region", region->Get());
   }
   return node;
}

template <> Node json::encode(om::Destination const& obj)
{
   Node node;

   om::Region3BoxedPtr region = obj.GetRegion();
   if (region) {
      node.set("region", region->Get());
   }
   om::Region3BoxedPtr adjacent = obj.GetAdjacent();
   if (adjacent) {
      node.set("adjacent", adjacent->Get());
   }
   node.set("auto_update_adjacent", obj.GetAutoUpdateAdjacent());
   node.set("allow_diagonal_adjacency", obj.GetAllowDiagonalAdjacency());
   return node;
}

template <> Node json::encode(om::CarryBlock const& obj)
{
   Node node;

   om::EntityPtr carrying = obj.GetCarrying().lock();
   if (carrying) {
      node.set("carrying", om::ObjectFormatter().GetPathToObject(carrying));
   } else {
      node.set("carrying", "");
   }   
   return node;
}

template <> Node json::encode(om::Effect const& obj)
{
   Node node;
   node.set("name", obj.GetName());
   return node;
}

template <> Node json::encode(om::EffectList const& obj)
{
   Node node;

   for (auto const& entry : obj.EachEffect()) {
      node.set(stdutil::ToString(entry.first), json::encode(*entry.second));
   }
   return node;
}

template <> Node json::encode(om::DataStore const& obj)
{
   return obj.GetJsonNode();
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

template <> json::Node json::encode(om::JsonBoxed const& obj)
{
   return obj.Get();
}
