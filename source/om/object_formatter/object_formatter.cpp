#include "radiant.h"
#include "object_formatter.h"
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

// templates...
#include "dm/store.h"
#include "om/om.h"
#include "dm/store.h"
#include "om/all_objects.h"
#include "om/all_components.h"

using namespace ::radiant;
using namespace ::radiant::om;

// xxx - It would be great if we could do this with a C++ 11 initializer, but VC doesn't
// support those yet.
static std::unordered_map<dm::ObjectType, std::function<JSONNode(const ObjectFormatter& f, dm::ObjectPtr)>> object_formatters_;
static void CreateDispatchTable();

template <typename T> JSONNode ToJson(const ObjectFormatter& f, T const& obj);

ObjectFormatter::ObjectFormatter()
{
}

JSONNode ObjectFormatter::ObjectToJson(dm::ObjectPtr obj) const
{
   CreateDispatchTable();

   if (obj) {
      auto i = object_formatters_.find(obj->GetObjectType());
      if (i != object_formatters_.end()) {
         JSONNode result = i->second(*this, obj);
         if (result.type() == JSON_NODE) {
            result.push_back(JSONNode("__self", GetPathToObject(obj)));
         }
         return result;
      }
   }
   // xxx: throw an exception...
   return JSONNode();
}

dm::ObjectPtr ObjectFormatter::GetObject(dm::Store const& store, std::string const& path) const
{
   std::regex exp("/o/stores/([^/]*)/objects/(\\d+)");
   std::smatch match;

   if (std::regex_match(path, match, exp)) {
      std::string store_name = match[1].str();
      if (store.GetName() == store_name) {
         std::string object_id = match[2].str();
         return store.FetchObject<dm::Object>(atoi(object_id.c_str()));
      }
   }
   return nullptr;
}

std::string ObjectFormatter::GetPathToObject(dm::ObjectPtr obj) const
{
   if (obj) {
      std::ostringstream output;
      output << "/o/stores/" << obj->GetStore().GetName() << "/objects/" << obj->GetObjectId();
      return output.str();
   }
   return "null";
}

template <> JSONNode ToJson(const ObjectFormatter& f, LuaComponents const& obj)
{
   JSONNode node;
   for (auto const& entry : obj.GetComponentMap()) {
      node.push_back(JSONNode(entry.first, f.GetPathToObject(entry.second)));
   }
   return node;
}

template <> JSONNode ToJson(const ObjectFormatter& f, Entity const& obj)
{
   JSONNode node;

   auto const& components = obj.GetComponents();
   for (auto const& entry : components) {
      dm::ObjectPtr c = entry.second;
      if (c->GetObjectType() == LuaComponentsObjectType) {
         JSONNode luac = ToJson(f, static_cast<LuaComponents&>(*c.get()));
         for (auto const& entry : luac) {
            node.push_back(entry);
         }
      } else {
         node.push_back(JSONNode(GetObjectNameLower(c), f.GetPathToObject(c)));
      }
   }
   return node;
}

template <> JSONNode ToJson(const ObjectFormatter& f, EntityContainer const& obj)
{
   JSONNode node;
   for (auto const& entry : obj.GetChildren()) {
      auto obj = entry.second.lock();
      if (obj) {
         node.push_back(JSONNode(stdutil::ToString(entry.first), f.GetPathToObject(obj)));
      }
   }
   return node;
}

template <> JSONNode ToJson(const ObjectFormatter& f, UnitInfo const& obj)
{
   JSONNode node;
   node.push_back(JSONNode("name", obj.GetDisplayName()));
   node.push_back(JSONNode("description", obj.GetDescription()));
   node.push_back(JSONNode("faction", obj.GetFaction()));
   node.push_back(JSONNode("icon", obj.GetIcon()));
   return node;
}

template <> JSONNode ToJson(const ObjectFormatter& f, DataBinding const& obj)
{
   return obj.GetJsonData();
}

template <> JSONNode ToJson(const ObjectFormatter& f, DataBindingP const& obj)
{
   return obj.GetJsonData();
}

template <> JSONNode ToJson(const ObjectFormatter& f, JsonStore const& obj)
{
   return obj.Get();
}

template <> JSONNode ToJson(const ObjectFormatter& f, ErrorBrowser const& obj)
{
   JSONNode node;

   JSONNode entries(JSON_ARRAY);
   entries.set_name("entries");
   for (auto const& e: obj.GetEntries()) {
      entries.push_back(JSONNode("", f.GetPathToObject(e)));
   }
   node.push_back(entries);

   return node;
}

#define OM_OBJECT(Cls, lower) \
   template <> JSONNode ToJson(const ObjectFormatter& f, Cls const& obj) { \
      JSONNode result; \
      result.push_back(JSONNode("error", "someone needs to implement " #Cls "::ToJson in the object formatter!")); \
      return result; \
   }
      OM_OBJECT(Effect,         effect)
      OM_OBJECT(Sensor,         sensor)
      OM_OBJECT(Aura,           aura)
      OM_OBJECT(TargetTable,        target_table)
      OM_OBJECT(TargetTableGroup,   target_table_group)
      OM_OBJECT(TargetTableEntry,   target_table_entry)
      OM_OBJECT(Clock,                 clock)
      OM_OBJECT(Mob,                   mob)
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

static void CreateDispatchTable()
{
   if (object_formatters_.empty()) {

#define OM_OBJECT(Cls, lower)                                       \
   object_formatters_[om::Cls ## ObjectType] =                      \
      [](const ObjectFormatter& f, dm::ObjectPtr obj) -> JSONNode { \
         return ToJson<om::Cls>(f, static_cast<Cls&>(*obj.get()));  \
      };
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS   
#undef OM_OBJECT

   }
}
