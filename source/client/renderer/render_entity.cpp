#include "pch.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include "utMath.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_entity_container.h"
#include "render_mob.h"
#include "render_terrain.h"
#include "render_destination.h"
#include "render_effect_list.h"
#include "render_render_info.h"
#include "render_carry_block.h"
#include "render_lua_component.h"
#include "render_region_collision_shape.h"
#include "render_vertical_pathing_region.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "dm/map_trace.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/entity_container.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/effect_list.ridl.h"
#include "om/components/render_info.ridl.h"
#include "om/components/carry_block.ridl.h"
#include "om/selection.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::client;

int RenderEntity::totalObjectCount_ = 0;

#define E_LOG(level)      LOG(renderer.entity, level)

RenderEntity::RenderEntity(H3DNode parent, om::EntityPtr entity) :
   entity_(entity),
   entity_id_(entity->GetObjectId()),
   initialized_(false)
{
   ASSERT(parent);

   E_LOG(5) << "creating render entity for object " << entity_id_;

   node_name_ = BUILD_STRING("(" << *entity << " store:" << entity->GetStoreId() 
                                 << " id:" << entity_id_ << ")");

   totalObjectCount_++;
   node_ = H3DNodeUnique(h3dAddGroupNode(parent, node_name_.c_str()));
   h3dSetNodeFlags(node_.get(), h3dGetNodeFlags(parent), true);

   skeleton_.SetSceneNode(node_.get());

   // xxx: convert to something more dm::Trace like...
   renderer_guard_ += Renderer::GetInstance().TraceSelected(node_.get(), std::bind(&RenderEntity::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void RenderEntity::FinishConstruction()
{
   auto entity = GetEntity();
   if (entity) {
      components_trace_ = entity->TraceComponents("render", dm::RENDER_TRACES)
                                       ->OnAdded([this](std::string const& name, std::shared_ptr<dm::Object> obj) {
                                          AddComponent(name, obj);
                                       })
                                       ->OnRemoved([this](std::string const& name) {
                                          RemoveComponent(name);
                                       })
                                       ->PushObjectState();
   }

   UpdateInvariantRenderers();
   initialized_ = true;
}

RenderEntity::~RenderEntity()
{
   Destroy();
   totalObjectCount_--;
}

void RenderEntity::Destroy()
{
   lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();

   E_LOG(7) << "destroying render entity " << node_name_;

   // xxx: share this with render_lua_component!!
   for (const auto& entry : lua_invariants_) {
      luabind::object obj = entry.second;
      if (obj) {
         try {
            luabind::object fn = obj["destroy"];
            if (fn) {
               fn(obj);
            }
         } catch (std::exception const& e) {
            E_LOG(1) << "error destroying component renderer: " << e.what();
            script->ReportCStackThreadException(obj.interpreter(), e);
         }
      }
   }
   lua_invariants_.clear();
   components_.clear();
}

void RenderEntity::SetParent(H3DNode parent)
{
   H3DNode node = node_.get();
   if (parent) {
      h3dSetNodeParent(node, parent);
      h3dSetNodeFlags(node, h3dGetNodeFlags(parent), true);
   } else {
      h3dSetNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);
   }
}

H3DNode RenderEntity::GetParent() const
{
   return h3dGetNodeParent(node_.get());
}

H3DNode RenderEntity::GetNode() const
{
   return node_.get();
}

std::string const& RenderEntity::GetName() const
{
   return node_name_;
}

int RenderEntity::GetTotalObjectCount()
{
   return totalObjectCount_;
}

// xxx: merge this with lua components...
void RenderEntity::UpdateInvariantRenderers()
{
   auto entity = entity_.lock();

   if (entity) {
      std::string uri = entity->GetUri();
      if (!uri.empty()) {
         auto const& res = res::ResourceManager2::GetInstance();
         json::Node json = res.LookupJson(uri);
         if (json.has("entity_data")) {
            for (auto const& entry : json.get_node("entity_data")) {
               std::string name = entry.name();
               size_t offset = name.find(':');
               if (offset != std::string::npos) {
                  lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
                  std::string modname = name.substr(0, offset);
                  std::string invariant_name = name.substr(offset + 1, std::string::npos);

                  json::Node manifest = res.LookupManifest(modname);
                  json::Node invariants = manifest.get_node("invariant_renderers");
                  std::string path = invariants.get<std::string>(invariant_name, "");
                  if (!path.empty()) {
                     luabind::object ctor = script->RequireScript(path);

                     std::weak_ptr<RenderEntity> re = shared_from_this();
                     luabind::object render_invariant;
                     try {
                        render_invariant = luabind::call_function<luabind::object>(ctor, re, script->JsonToLua(entry));
                     } catch (std::exception const& e) {
                        script->ReportCStackThreadException(ctor.interpreter(), e);
                        continue;
                     }
                     lua_invariants_[name] = render_invariant;
                  }
               }
            }
         }
      }
   }
}

void RenderEntity::AddComponent(std::string const& name, std::shared_ptr<dm::Object> value)
{
   ASSERT(value);

   auto entity = entity_.lock();
   if (entity) {
      switch(value->GetObjectType()) {
         case om::TerrainObjectType: {
            om::TerrainPtr terrain = std::static_pointer_cast<om::Terrain>(value);
            components_[name] = std::make_shared<RenderTerrain>(*this, terrain);
            break;
         }
         case om::MobObjectType: {
            om::MobPtr mob = std::static_pointer_cast<om::Mob>(value);
            components_[name] = std::make_shared<RenderMob>(*this, mob);
            break;
         }
         case om::RenderInfoObjectType: {
            om::RenderInfoPtr ri = std::static_pointer_cast<om::RenderInfo>(value);
            components_[name] = std::make_shared<RenderRenderInfo>(*this, ri);
            break;
         }
         case om::EntityContainerObjectType: {
            om::EntityContainerPtr container = std::static_pointer_cast<om::EntityContainer>(value);
            components_[name] = std::make_shared<RenderEntityContainer>(*this, container);
            break;
         }
         case om::DestinationObjectType: {
            om::DestinationPtr destination = std::static_pointer_cast<om::Destination>(value);
            components_[name] = std::make_shared<RenderDestination>(*this, destination);
            break;
         }
         case om::EffectListObjectType: {
            om::EffectListPtr el = std::static_pointer_cast<om::EffectList>(value);
            components_[name] = std::make_shared<RenderEffectList>(*this, el);
            break;
         }
         case om::CarryBlockObjectType: {
            om::CarryBlockPtr renderRegion = std::static_pointer_cast<om::CarryBlock>(value);
            components_[name] = std::make_shared<RenderCarryBlock>(*this, renderRegion);
            break;
         }
         case om::VerticalPathingRegionObjectType: {
            om::VerticalPathingRegionPtr obj = std::static_pointer_cast<om::VerticalPathingRegion>(value);
            components_[name] = std::make_shared<RenderVerticalPathingRegion>(*this, obj);
            break;
         }
         case om::RegionCollisionShapeObjectType: {
            om::RegionCollisionShapePtr obj = std::static_pointer_cast<om::RegionCollisionShape>(value);
            components_[name] = std::make_shared<RenderRegionCollisionShape>(*this, obj);
            break;
         }
         case om::DataStoreObjectType: {
            om::DataStorePtr obj = std::static_pointer_cast<om::DataStore>(value);
            components_[name] = std::make_shared<RenderLuaComponent>(*this, name, obj);
            break;
         }
      }
   }
}


void RenderEntity::RemoveComponent(std::string const& name)
{
   components_.erase(name);
}

void RenderEntity::OnSelected(om::Selection& sel, const csg::Ray3& ray,
                              const csg::Point3f& intersection, const csg::Point3f& normal)
{
   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      // xxx: don't select authored objects!
      if (entity->GetStoreId() != 2) {
         E_LOG(1) << "selected authoring entity " << *entity << ".  Ignoring";
         return;
      }
      sel.AddEntity(entity_id_);
   }
}

void RenderEntity::Show(bool show)
{
   int mask = H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery;
   int flags = h3dGetNodeFlags(node_.get());

   if (show) {
      flags &= ~mask;
   } else {
      flags |= mask;
   }
   h3dSetNodeFlags(node_.get(), flags, true);
}

void RenderEntity::SetSelected(bool selected)
{
   int mask = H3DNodeFlags::Selected;
   int flags = h3dGetNodeFlags(node_.get());

   if (selected) {
      flags |= mask;
   } else {
      flags &= ~mask;
   }
   h3dSetNodeFlags(node_.get(), flags, true);
}

dm::ObjectId RenderEntity::GetObjectId() const
{
   return entity_id_;
}

// xxx: omg, get rid of this!
void RenderEntity::SetModelVariantOverride(bool enabled, std::string const& variant)
{
   std::shared_ptr<RenderRenderInfo> ri = std::static_pointer_cast<RenderRenderInfo>(components_["render_info"]);
   if (ri) {
      ri->SetModelVariantOverride(enabled, variant);
   }
}
