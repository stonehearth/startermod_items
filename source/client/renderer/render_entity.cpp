#include "pch.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include "utMath.h"
#include "renderer.h"
#include "render_build_order.h"
#include "render_build_orders.h"
#include "render_entity.h"
#include "render_entity_container.h"
#include "render_grid.h"
#include "render_carry_block.h"
#include "render_mob.h"
#include "render_terrain.h"
#include "render_scaffolding.h"
#include "render_destination.h"
#include "render_rig.h"
#include "render_paperdoll.h"
#include "render_effect_list.h"
#include "render_render_region.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/carry_block.h"
#include "om/components/entity_container.h"
#include "om/components/render_grid.h"
#include "om/components/render_rig.h"
#include "om/components/stockpile_designation.h"
#include "om/components/build_orders.h"
#include "om/components/terrain.h"
#include "om/components/paperdoll.h"
#include "om/components/lua_components.h"
#include "om/components/scaffolding.h"
#include "om/components/effect_list.h"
#include "om/components/render_info.h"
#include "om/selection.h"
#include "lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::client;

int RenderEntity::totalObjectCount_ = 0;

RenderEntity::RenderEntity(H3DNode parent, om::EntityPtr entity) :
   entity_(entity),
   initialized_(false),
   displayIconic_(-1)
{
   ASSERT(parent);


   totalObjectCount_++;
   om::EntityId id = entity->GetObjectId();

   std::ostringstream name;
   name << "RenderEntity " << entity->GetDebugName() << " (" << entity->GetStoreId() << ", " << id << ")";

   // LOG(WARNING) << "creating new entity " << name.str() << ".";

   node_ = h3dAddGroupNode(parent, name.str().c_str());
   h3dSetNodeFlags(node_, h3dGetNodeFlags(parent), true);

   skeleton_.SetSceneNode(node_);

   auto added = std::bind(&RenderEntity::AddComponent, this, std::placeholders::_1, std::placeholders::_2);
   auto removed = std::bind(&RenderEntity::RemoveComponent, this, std::placeholders::_1);

   tracer_ += entity->GetComponents().TraceMapChanges("render entity components", added, removed);
   tracer_ += Renderer::GetInstance().TraceSelected(node_, std::bind(&RenderEntity::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void RenderEntity::FinishConstruction()
{
   UpdateComponents();
   initialized_ = true;
   CreateRenderRigs();
}

RenderEntity::~RenderEntity()
{
   if (node_) {
      h3dRemoveNode(node_);
   }
   totalObjectCount_--;
}

void RenderEntity::SetParent(H3DNode parent)
{
   if (parent) {
      h3dSetNodeParent(node_, parent);
      h3dSetNodeFlags(node_, h3dGetNodeFlags(parent), true);
   } else {
      h3dSetNodeFlags(node_, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);
   }
}

H3DNode RenderEntity::GetParent() const
{
   return h3dGetNodeParent(node_);
}

H3DNode RenderEntity::GetNode() const
{
   return node_;
}

std::string RenderEntity::GetName() const
{
   return std::string(h3dGetNodeParamStr(node_, H3DNodeParams::NameStr));
}

int RenderEntity::GetTotalObjectCount()
{
   return totalObjectCount_;
}

void RenderEntity::UpdateComponents()
{
   auto entity = entity_.lock();
   if (entity) {
      components_.clear();
      for (const auto& entry : entity->GetComponents()) {
         AddComponent(entry.first, entry.second);
      }
   }
}

void RenderEntity::CreateRenderRigs()
{
   if (initialized_) {
      auto entity = entity_.lock();
      if (entity) {
         auto renderRig = entity->GetComponent<om::RenderRig>();
         auto renderRigIconic = entity->GetComponent<om::RenderRigIconic>();
         auto renderInfo = entity->GetComponent<om::RenderInfo>();
         bool renderIconic;
         
         // This is annoyingly weird.  We want to be able to change the iconic vs. non-iconic
         // representation of an item at an arbitrary interpolation frame to support seamless
         // animation.  This requires overriding the value of the iconic flag in the render
         // info until we can get an update from the server.
         if (displayIconic_ != -1) {
            renderIconic = displayIconic_ != 0;
         } else {
            renderIconic = renderInfo ? renderInfo->GetDisplayIconic() : false;
         }

         if (renderIconic && renderRigIconic) {
            components_[om::RenderRigObjectType] = nullptr;
            if (!components_[om::RenderRigIconicObjectType]) {
               components_[om::RenderRigIconicObjectType] = std::make_shared<RenderRig>(*this, renderRigIconic);
            }
         } else if (renderRig) {
            if (!components_[om::RenderRigObjectType]) {
               components_[om::RenderRigObjectType] = std::make_shared<RenderRig>(*this, renderRig);
            }
            components_[om::RenderRigIconicObjectType] = nullptr;
         } else {
            components_[om::RenderRigObjectType] = nullptr;
            components_[om::RenderRigIconicObjectType] = nullptr;
         }
      }
   }
}

void RenderEntity::AddComponent(dm::ObjectType key, std::shared_ptr<dm::Object> value)
{
   ASSERT(value);

   auto entity = entity_.lock();
   if (entity) {
      switch(key) {
         ASSERT(key == value->GetObjectType());
         case om::RenderInfoObjectType: {
            om::RenderInfoPtr renderInfo = std::static_pointer_cast<om::RenderInfo>(value);
            tracer_ += renderInfo->TraceRecordField("iconic", "render render_info", [=]() {
               CreateRenderRigs();
            });
            break;
         };

         case om::TerrainObjectType: {
            om::TerrainPtr terrain = std::static_pointer_cast<om::Terrain>(value);
            components_[key] = std::make_shared<RenderTerrain>(*this, terrain);
            break;
         }
         case om::RenderGridObjectType: {
            om::RenderGridPtr render_grid = std::static_pointer_cast<om::RenderGrid>(value);
            components_[key] = std::make_shared<RenderGrid>(*this, render_grid);
            break;
         }
         case om::CarryBlockObjectType: {
            om::CarryBlockPtr carry_block = std::static_pointer_cast<om::CarryBlock>(value);
            components_[key] = std::make_shared<RenderCarryBlock>(*this, carry_block);
            break;
         }
         case om::MobObjectType: {
            om::MobPtr mob = std::static_pointer_cast<om::Mob>(value);
            components_[key] = std::make_shared<RenderMob>(*this, mob);
            break;
         }
         case om::RenderRigObjectType:
         case om::RenderRigIconicObjectType: {
            CreateRenderRigs();
            break;
         }
         case om::EntityContainerObjectType: {
            om::EntityContainerPtr container = std::static_pointer_cast<om::EntityContainer>(value);
            components_[key] = std::make_shared<RenderEntityContainer>(*this, container);
            break;
         }
         case om::DestinationObjectType: {
            om::DestinationPtr stockpile = std::static_pointer_cast<om::Destination>(value);
            components_[key] = std::make_shared<RenderDestination>(*this, stockpile);
            break;
         }
         case om::LuaComponentsObjectType: {
            AddLuaComponents(std::static_pointer_cast<om::LuaComponents>(value));
            break;
         }
         case om::WallObjectType:
         case om::PostObjectType: {
            om::RegionBuildOrderPtr buildOrder = std::static_pointer_cast<om::RegionBuildOrder>(value);
            components_[key] = std::make_shared<RenderBuildOrder>(*this, buildOrder);
            break;
         }
         case om::ScaffoldingObjectType:{
            om::ScaffoldingPtr scaffolding = std::static_pointer_cast<om::Scaffolding>(value);
            components_[key] = std::make_shared<RenderScaffolding>(*this, scaffolding);
            break;
         }
         case om::BuildOrdersObjectType: {
            om::BuildOrdersPtr bo = std::static_pointer_cast<om::BuildOrders>(value);
            components_[key] = std::make_shared<RenderBuildOrders>(*this, bo);
            break;
         }
         case om::EffectListObjectType: {
            om::EffectListPtr el = std::static_pointer_cast<om::EffectList>(value);
            components_[key] = std::make_shared<RenderEffectList>(*this, el);
            break;
         }
         case om::PaperdollObjectType: {
            om::PaperdollPtr paperdoll = std::static_pointer_cast<om::Paperdoll>(value);
            components_[key] = std::make_shared<RenderPaperdoll>(*this, paperdoll);
            break;
         }
         case om::RenderRegionObjectType: {
            om::RenderRegionPtr renderRegion = std::static_pointer_cast<om::RenderRegion>(value);
            components_[key] = std::make_shared<RenderRenderRegion>(*this, renderRegion);
            break;
         }
      }
   }
}

void RenderEntity::AddLuaComponents(om::LuaComponentsPtr lua_components)
{
   // xxx: gotta trace to catch lua components added after the initial push
   for (auto const& entry : lua_components->GetComponentMap()) {
      std::string const& name = entry.first;
      size_t offset = name.find(':');
      if (offset != std::string::npos) {
         std::string modname = name.substr(0, offset);
         std::string component_name = name.substr(offset + 1, std::string::npos);

         auto const& res = resources::ResourceManager2::GetInstance();
         json::ConstJsonObject const& manifest = res.LookupManifest(modname);
         std::string path;
         try {
            path = manifest["component_renderers"][component_name].as_string();
         } catch (json::Exception&) {
            continue;
         }
         lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
         luabind::object ctor = script->LuaRequire(path);

         std::weak_ptr<RenderEntity> re = shared_from_this();
         luabind::object render_component;
         try {
            luabind::object json = script->JsonToLua(entry.second->ToJson());
            render_component = script->CallFunction<luabind::object>(ctor, re, json);
         } catch (std::exception const& e) {
            LOG(WARNING) << e.what();
            continue;
         }
         lua_components_[name] = render_component;
      }
   }
}

void RenderEntity::RemoveComponent(dm::ObjectType key)
{
   components_.erase(key);
}

void RenderEntity::OnSelected(om::Selection& sel, const math3d::ray3& ray,
                              const math3d::point3& intersection, const math3d::point3& normal)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      // xxx: don't select authored objects!
      ASSERT(entity->GetStoreId() == 2);
      if (true || (mob && mob->IsSelectable())) {
         sel.AddEntity(entity->GetObjectId());
      }
   }
}

void RenderEntity::Show(bool show)
{
   int mask = H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery;
   int flags = h3dGetNodeFlags(node_);

   if (show) {
      flags &= ~mask;
   } else {
      flags |= mask;
   }
   h3dSetNodeFlags(node_, flags, true);
}

void RenderEntity::SetSelected(bool selected)
{
   int mask = H3DNodeFlags::Selected;
   int flags = h3dGetNodeFlags(node_);

   if (selected) {
      flags |= mask;
   } else {
      flags &= ~mask;
   }
   h3dSetNodeFlags(node_, flags, true);
}

om::EntityId RenderEntity::GetEntityId() const
{
   auto entity = entity_.lock();
   return entity ? entity->GetEntityId() : 0;
}

bool RenderEntity::ShowDebugRegions() const
{
   return true;
}


void RenderEntity::SetDisplayIconicOveride(bool value)
{
   displayIconic_ = value ? 1 : 0;
   CreateRenderRigs();
}

void RenderEntity::ClearDisplayIconicOveride()
{
   displayIconic_= -1;
   CreateRenderRigs();
}

