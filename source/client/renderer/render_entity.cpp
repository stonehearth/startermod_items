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
#include "render_attached_items.h"
#include "render_lua_component.h"
#include "render_region_collision_shape.h"
#include "render_vertical_pathing_region.h"
#include "render_sensor_list.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "dm/map_trace.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/entity_container.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/effect_list.ridl.h"
#include "om/components/render_info.ridl.h"
#include "om/components/attached_items.ridl.h"
#include "om/components/sensor_list.ridl.h"
#include "om/selection.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::client;

int RenderEntity::totalObjectCount_ = 0;

#define E_LOG(level)      LOG(renderer.entity, level)

// Contains a map from H3DNodes to RenderEntities.  This map is maintained by the
// RenderEntities, adding themselves to the map in the constructor and removing
// themselves in the destructor.  Since we only render on one thread for now,
// we don't need any locking of synchronization on the map.
static std::unordered_map<H3DNode, std::weak_ptr<RenderEntity>> nodeToRenderEntity;

RenderEntity::RenderEntity(H3DNode parent, om::EntityPtr entity) :
   entity_(entity),
   entity_id_(entity->GetObjectId()),
   initialized_(false),
   visible_override_(true)
{
   ASSERT(parent);

   E_LOG(5) << "creating render entity for object " << entity_id_;

   node_name_ = BUILD_STRING("(" << *entity << " store:" << entity->GetStoreId() 
                                 << " id:" << entity_id_ << ")");

   totalObjectCount_++;
   node_ = H3DNodeUnique(h3dAddGroupNode(parent, node_name_.c_str()));

   skeleton_.SetSceneNode(node_.get());

   // xxx: convert to something more dm::Trace like...
   selection_guard_ = Renderer::GetInstance().SetSelectionForNode(node_.get(), entity);

   query_flags_ = 0;
}

void RenderEntity::FinishConstruction()
{
   nodeToRenderEntity[node_.get()] = shared_from_this();

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

      lua_components_trace_ = entity->TraceLuaComponents("render", dm::RENDER_TRACES)
                                       ->OnAdded([this](std::string const& name, om::DataStorePtr obj) {
                                          AddLuaComponent(name, obj);
                                       })
                                       ->OnRemoved([this](std::string const& name) {
                                          RemoveComponent(name);
                                       })
                                       ->PushObjectState();
   }

   // Make sure we always have a RenderRenderInfo component, since that's where things like material
   // override and visible override get resolved.  If someone on the server later adds a render_info
   // component, this copy will get blown away (which is just fine!).
   if (!components_["render_info"]) {
      components_["render_info"] = std::make_shared<RenderRenderInfo>(*this, nullptr);
   }

   UpdateInvariantRenderers();
   initialized_ = true;
}

RenderEntity::~RenderEntity()
{
   nodeToRenderEntity.erase(node_.get());

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
   if (!parent) {
      parent = RenderNode::GetUnparentedNode();
   }
   h3dSetNodeParent(node_.get(), parent);
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
         res.LookupJson(uri, [&](const json::Node& json) {
            if (json.has("entity_data")) {
               for (auto const& entry : json.get_node("entity_data")) {
                  std::string name = entry.name();
                  size_t offset = name.find(':');
                  if (offset != std::string::npos) {
                     lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
                     std::string modname = name.substr(0, offset);
                     std::string invariant_name = name.substr(offset + 1, std::string::npos);

                     std::string path;
                     res.LookupManifest(modname, [&](const res::Manifest& manifest) {
                        const json::Node invariants = manifest.get_node("invariant_renderers");
                        path = invariants.get<std::string>(invariant_name, "");
                     });

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
         });
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
         case om::AttachedItemsObjectType: {
            om::AttachedItemsPtr renderRegion = std::static_pointer_cast<om::AttachedItems>(value);
            components_[name] = std::make_shared<RenderAttachedItems>(*this, renderRegion);
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
         case om::SensorListObjectType: {
            om::SensorListPtr obj = std::static_pointer_cast<om::SensorList>(value);
            components_[name] = std::make_shared<RenderSensorList>(*this, obj);
            break;
         }
      }
   }
}

void RenderEntity::AddLuaComponent(std::string const& name, om::DataStorePtr obj)
{
   auto i = components_.find(name);
   if (i != components_.end()) {
      // the component changed.  we could make the existing lua component renderer
      // smart enough to re-initialize itself with the new component data, but this
      // happens so infrequently it's not worth it to push the additional complexity
      // onto modders.  just nuke the one we have and create another one.
      components_.erase(i);
   }
   components_[name] = std::make_shared<RenderLuaComponent>(*this, name, obj);
}


void RenderEntity::RemoveComponent(std::string const& name)
{
   components_.erase(name);
}

void RenderEntity::SetSelected(bool selected)
{
   h3dTwiddleNodeFlags(node_.get(), H3DNodeFlags::Selected, selected, true);
}

dm::ObjectId RenderEntity::GetObjectId() const
{
   return entity_id_;
}

void RenderEntity::SetRenderInfoDirtyBits(int bits)
{
   std::shared_ptr<RenderRenderInfo> ri = std::static_pointer_cast<RenderRenderInfo>(components_["render_info"]);
   if (ri) {
      ri->SetDirtyBits(bits);
   }
}

void RenderEntity::SetModelVariantOverride(std::string const& variant)
{
   model_variant_override_ = variant;
   SetRenderInfoDirtyBits(RenderRenderInfo::MODEL_DIRTY);
}

void RenderEntity::SetMaterialOverride(std::string const& material)
{
   material_override_ = material;
   SetRenderInfoDirtyBits(RenderRenderInfo::MATERIAL_DIRTY);
}

void RenderEntity::SetVisibleOverride(bool visible)
{
   visible_override_ = visible;
   SetRenderInfoDirtyBits(RenderRenderInfo::VISIBLE_DIRTY);
}

std::string const RenderEntity::GetMaterialPathFromKind(std::string const& matKind) const
{
   std::string matPath;
   auto entity = entity_.lock();

   if (entity) {
      auto lookupCallback = [&matKind, &matPath](JSONNode const& data) {
         json::Node n(data);
         matPath = n.get("entity_data.stonehearth:render_materials." + matKind,
            "materials/voxel.material.xml");
      };
      try {
         res::ResourceManager2::GetInstance().LookupJson(entity->GetUri(), lookupCallback);
      } catch (std::exception const&) {
      }
   }
   return matPath;
}

void RenderEntity::AddQueryFlag(int flag)
{
   query_flags_ |= flag;
}

void RenderEntity::RemoveQueryFlag(int flag)
{
   query_flags_ &= ~flag;
}

bool RenderEntity::HasQueryFlag(int flag) const
{
   return (query_flags_ & flag) != 0;
}

void RenderEntity::ForAllSceneNodes(std::function<void(H3DNode node)> fn)
{
   ForAllSceneNodes(node_.get(), fn);
}


/*
 * -- RenderEntity::ForAllSceneNodes
 *
 * Call `fn` for every node which "belongs" to this Entity.  We define "belongs" to
 * mean the node is a descendant of the group node created for this RenderEntity,
 * but not a descendant of any of this RenderEntity's children.  For example, if
 * 3 swords are contained in a chest, calling ForAllSceneNodes on the node for the
 * chest will *not* iterate through the nodes of the swords.
 */
void RenderEntity::ForAllSceneNodes(H3DNode node, std::function<void(H3DNode node)> fn)
{
   // Look in the `nodeToRenderEntity` map to see if this is the root node for
   // a RenderEntity.  If so, and that RenderEntity isn't us, then we must have
   // descended down to a child Entity which is wired up underneath us in the
   // scene graph.  Stop recurring!
   auto i = nodeToRenderEntity.find(node);
   if (i != nodeToRenderEntity.end()) {
      RenderEntityPtr re = i->second.lock();
      if (re.get() != this) {
         return;
      }
   }
   // Call the callback for this node
   fn(node);

   // Recur through all the children
   int j = 0;
   while (true) {
      H3DNode child = h3dGetNodeChild(node, j++);
      if (child == 0) {
         break;
      }
      ForAllSceneNodes(child, fn);
   }
}

std::string const& RenderEntity::GetMaterialOverride() const
{
   return material_override_;
}

std::string const& RenderEntity::GetModelVariantOverride() const
{
   return model_variant_override_;
}

bool RenderEntity::GetVisibleOverride() const
{
   return visible_override_;
}

