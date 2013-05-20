#include "pch.h"
#include "stonehearth.h"
#include "entity.h"
#include "radiant_json.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

static void Ignore(om::EntityPtr entity, JSONNode const& obj)
{
}

static void InitUnitInfoComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto unitInfo = entity->AddComponent<UnitInfo>();

   unitInfo->SetDisplayName(obj["name"].as_string());
   unitInfo->SetDescription(obj["description"].as_string());
   unitInfo->SetFaction(json::get<std::string>(obj, "faction", ""));
}

static void InitWeaponInfoComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto weaponInfo = entity->AddComponent<WeaponInfo>();

   weaponInfo->SetRange(json::get<float>(obj, "range", 1.0f));
   weaponInfo->SetDamageString(json::get<std::string>(obj, "damage", "1"));
}

static void InitPortalComponent(om::EntityPtr entity, JSONNode const& obj)
{
   entity->AddComponent<Portal>()->SetPortal(resources::ParsePortal(obj));

   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false); // xxx: this is not a great place to do this... specify it in the json!
}

static void InitResourceNodeComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto node = entity->AddComponent<ResourceNode>();

   node->SetDurability(obj["durability"].as_int());
   node->SetResource(obj["resource"].as_string());
}

static void SetAnimationTable(om::EntityPtr entity, JSONNode const& obj)
{
   auto renderRig = entity->AddComponent<RenderRig>();
   renderRig->SetAnimationTable(obj.as_string());
}

static void InitAttributesComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto attributes = entity->AddComponent<om::Attributes>();
   for (auto const& e : obj) {
      attributes->SetAttribute(e.name(), e.as_int());
   }
}

static void InitSensorListComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto sensorList = entity->AddComponent<om::SensorList>();
   for (auto const& e : obj["sensors"]) {
      sensorList->AddSensor(e.name(), e["radius"].as_int());
   }
}

static void InitRenderRigComponent(om::RenderRigPtr renderRig, JSONNode const& obj)
{
   renderRig->SetScale(json::get<float>(obj, "scale", 0.1f));

   for (auto const& e : obj["models"]) {
      if (e.type() == JSON_STRING) {
         renderRig->AddRig(e.as_string());
      } else if (e.type() == JSON_NODE) {
         if (e["type"].as_string() == "one_of") {
            JSONNode const& items = e["items"];
            uint c = rand() * items.size() / RAND_MAX;
            ASSERT(c < items.size());
            renderRig->AddRig(items.at(c).as_string());
         }
      }
   }
}

static void InitRenderRigComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto renderRig = entity->AddComponent<RenderRig>();
   InitRenderRigComponent(renderRig, obj);
}

static void InitRenderRigIconicComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto renderRig = entity->AddComponent<RenderRigIconic>();
   InitRenderRigComponent(renderRig, obj);
}

static void InitFixtureComponent(om::EntityPtr entity, JSONNode const& obj)
{   
   auto fixture = entity->AddComponent<Fixture>();
   fixture->SetItemKind(obj["item"].as_string());
}

static void InitItemComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto item = entity->AddComponent<Item>();

   int count = obj["stacks"].as_int();
   item->SetStacks(count);
   item->SetMaxStacks(count);
   item->SetMaterial(obj["material"].as_string());

   // Should we put the flags on the entity??
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);
}

static void InitActionListComponent(om::EntityPtr entity, JSONNode const& obj)
{
   auto actionList = entity->AddComponent<ActionList>();
   for (unsigned int i = 0; i < obj.size(); i++) {
      JSONNode const& e = obj.at(i);
      actionList->AddAction(e.as_string());
   }
}

om::EntityPtr Stonehearth::CreateEntity(dm::Store& store, std::string uri)
{
   static struct {
      const char* name;
      void (*fn)(om::EntityPtr, JSONNode const&);
   } components[] = {
      { "portal",          InitPortalComponent },
      { "fixture",         InitFixtureComponent },
      { "unit_info",       InitUnitInfoComponent },
      { "weapon_info",     InitWeaponInfoComponent },
      { "armor_info",      Ignore},
      { "action_list",     InitActionListComponent },
      { "render_rig",      InitRenderRigComponent },
      { "render_rig_iconic", InitRenderRigIconicComponent },
      { "item",            InitItemComponent },
      { "resource_node",   InitResourceNodeComponent },
      { "animation_table", SetAnimationTable },
      { "attributes",      InitAttributesComponent},
      { "sensor_list",     InitSensorListComponent},
      { "ai",              Ignore },
      { "type",            Ignore },
      { "scripts",         Ignore },
      { "data",            Ignore },
   };

   om::EntityPtr entity = store.AllocObject<om::Entity>();
   std::ostringstream debug_name;
   debug_name << "(" << entity->GetObjectId() << ") " << uri;
   entity->SetDebugName(debug_name.str());

   if (!uri.empty()) {
      auto resource = resources::ResourceManager2::GetInstance().Lookup<resources::DataResource>(uri);
      ASSERT(resource);

      if (resource) {
         JSONNode const& obj = resource->GetJson();

         entity->SetResourceUri(uri);
         ASSERT(obj["type"].as_string() == "entity");
         for (auto const& entry : obj) {
            bool handled = false;
            for (const auto& component : components) {
               std::string name = entry.name();
               if (name == component.name) {
                  component.fn(entity, entry);
                  handled = true;
                  break;
               }
            }
            if (!handled) {
               LOG(WARNING) << "unknown field '" << entry.name() << "' in definition for entity '" << uri << "'.";
               ASSERT(handled);
            }
         }
      }
   }

   return entity;
}

csg::Region3 Stonehearth::ComputeStandingRegion(const csg::Region3& r, int height)
{
   csg::Region3 standing;
   for (const csg::Cube3& c : r) {
      auto p0 = c.GetMin();
      auto p1 = c.GetMax();
      auto w = p1[0] - p0[0], h = p1[2] - p0[2];
      standing.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(w, height, 0)));  // top
      standing.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, height, h)));  // left
      standing.Add(csg::Cube3(p0 + csg::Point3(0, 0, h), p0 + csg::Point3(w, height, h + 1)));  // bottom
      standing.Add(csg::Cube3(p0 + csg::Point3(w, 0, 0), p0 + csg::Point3(w + 1, height, h)));  // right
   }
   standing.Optimize();

   return standing;
}
