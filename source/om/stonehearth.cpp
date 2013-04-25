#include "pch.h"
#include "stonehearth.h"
#include "entity.h"
#include "resources/rig.h"
#include "resources/region2d.h"
#include "resources/res_manager.h"
#include "resources/array_resource.h"
#include "resources/data_resource.h"

using namespace ::radiant;
using namespace ::radiant::om;

static void Ignore(om::EntityPtr entity, resources::ResourcePtr resource)
{
}

static void InitUnitInfoComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto unitInfo = entity->AddComponent<UnitInfo>();

   unitInfo->SetDisplayName(obj->GetString("name"));
   unitInfo->SetDescription(obj->GetString("description"));
   unitInfo->SetFaction(obj->GetString("faction"));
}

static void InitWeaponInfoComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto weaponInfo = entity->AddComponent<WeaponInfo>();

   weaponInfo->SetRange(obj->GetFloat("range"));
   weaponInfo->SetDamageString(obj->GetString("damage"));
}

static void InitPortalComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto portalRegion = std::dynamic_pointer_cast<resources::Region2d>(resource);
   if (portalRegion) {
      entity->AddComponent<Portal>()->SetPortal(*portalRegion);
   }

   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);
}

static void InitResourceNodeComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto node = entity->AddComponent<ResourceNode>();

   node->SetDurability(obj->GetInteger("durability"));
   node->SetResource(obj->GetString("resource"));
}

static void SetAnimationTable(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto tableName = std::dynamic_pointer_cast<resources::StringResource>(resource);
   if (tableName) {
      auto renderRig = entity->AddComponent<RenderRig>();
      renderRig->SetAnimationTable(tableName->GetValue());
   }
}

static void InitAttributesComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto attributes = entity->AddComponent<om::Attributes>();
   if (obj) {
      for (const auto& entry : obj->GetContents()) {
         std::string name = entry.first;
         auto value = std::dynamic_pointer_cast<resources::NumberResource>(entry.second);
         if (value) {
            attributes->SetAttribute(name, (int)value->GetValue());
         }
      }
   }
}

static void InitSensorListComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto sensorList = entity->AddComponent<om::SensorList>();
   if (obj) {
      auto sensors = obj->Get<resources::ObjectResource>("sensors");
      if (sensors) {
         for (const auto& entry : sensors->GetContents()) {
            std::string name = entry.first;
            auto value = std::dynamic_pointer_cast<resources::ObjectResource>(entry.second);
            if (value) {
               int radius = value->GetInteger("radius");
               sensorList->AddSensor(name, radius);
            }
         }
      }
   }
}

static void InitRenderRigComponent(resources::ObjectResourcePtr obj, om::RenderRigPtr renderRig)
{
   renderRig->SetScale(obj->GetFloat("scale", 0.1f));

   auto models = obj->Get<resources::ArrayResource>("models");
   if (models) {
      for (const auto& item : models->GetContents()) {
         if (item->GetType() == resources::Resource::RIG) {
            auto rig = std::static_pointer_cast<resources::Rig>(item);
            renderRig->AddRig(rig->GetResourceIdentifier());
         } else if (item->GetType() == resources::Resource::STRING) {
            auto str = std::static_pointer_cast<resources::StringResource>(item);
            renderRig->AddRig(str->GetValue());
         } else if (item->GetType() == resources::Resource::ONE_OF) {
            auto data = std::static_pointer_cast<resources::DataResource>(item);
            const JSONNode& items = data->GetJson()["items"];
            if (items.type() == JSON_ARRAY) {
               int c = rand() * items.size() / RAND_MAX;
               if (c == items.size()) {
                  c = items.size() - 1;
               }
               const JSONNode& model = items.at(c);
               if (model.type() == JSON_STRING) {
                  std::string name = model.as_string();
                  if (!name.empty()) {
                     renderRig->AddRig(name);
                  }
               }
            }
         }
      }
   }
}

static void InitRenderRigComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);

   if (obj) {
      auto renderRig = entity->AddComponent<RenderRig>();
      InitRenderRigComponent(obj, renderRig);
   }
}

static void InitRenderRigIconicComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);

   if (obj) {
      auto renderRig = entity->AddComponent<RenderRigIconic>();
      InitRenderRigComponent(obj, renderRig);
   }
}

static void InitFixtureComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto fixture = entity->AddComponent<Fixture>();

   fixture->SetItemKind(obj->GetString("item"));
}

static void InitItemComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto obj = std::dynamic_pointer_cast<resources::ObjectResource>(resource);
   auto item = entity->AddComponent<Item>();

   int count = obj->GetInteger("stacks");
   item->SetStacks(count);
   item->SetMaxStacks(count);
   item->SetMaterial(obj->GetString("material"));

   // Should we put the flags on the entity??
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);
}

static void InitActionListComponent(om::EntityPtr entity, resources::ResourcePtr resource)
{
   auto arr = std::dynamic_pointer_cast<resources::ArrayResource>(resource);
   auto actionList = entity->AddComponent<ActionList>();
   for (const auto& entry : arr->GetContents()) {
      auto action = std::dynamic_pointer_cast<resources::StringResource>(entry);
      if (action) {
         actionList->AddAction(action->GetValue());
      }
   }
}

om::EntityPtr Stonehearth::CreateEntity(dm::Store& store, std::string kind)
{
   static struct {
      const char* name;
      void (*fn)(om::EntityPtr, resources::ResourcePtr);
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
   };

   om::EntityPtr entity = store.AllocObject<om::Entity>();
   auto obj = resources::ResourceManager2::GetInstance().Lookup<resources::ObjectResource>(kind);

   if (obj) {
      ASSERT(obj->GetString("type") == "entity");
      for (const auto& entry : obj->GetContents()) {
         bool handled = false;
         for (const auto& component : components) {
            if (entry.first == component.name) {
               component.fn(entity, entry.second);
               handled = true;
               break;
            }
         }
         ASSERT(handled);
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
