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
#include "om/components/sensor_list.ridl.h"
#include "om/selection.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::client;

int RenderEntity::totalObjectCount_ = 0;

#define E_LOG(level)      LOG(renderer.entity, level)


static void MoveSceneNode(H3DNode node, const csg::Transform& t)
{
   csg::Matrix4 m(t.orientation);
   m[12] = (float)t.position.x;
   m[13] = (float)t.position.y;
   m[14] = (float)t.position.z;

   bool result = h3dSetNodeTransMat(node, m.get_float_ptr());
   if (!result) {
      E_LOG(9) << "failed to set transform on node " << node << ".";
   }
}

// Contains a map from H3DNodes to RenderEntities.  This map is maintained by the
// RenderEntities, adding themselves to the map in the constructor and removing
// themselves in the destructor.  Since we only render on one thread for now,
// we don't need any locking of synchronization on the map.
static std::unordered_map<H3DNode, std::weak_ptr<RenderEntity>> nodeToRenderEntity;

RenderEntity::RenderEntity(H3DNode parent, om::EntityPtr entity) :
   entity_(entity),
   entity_id_(entity->GetObjectId()),
   initialized_(false),
   destroyed_(false),
   skeleton_(*this),
   visible_override_ref_count_(0),
   parentOverride_(false)
{
   ASSERT(parent);

   E_LOG(5) << "creating render entity for object " << entity_id_;

   node_name_ = BUILD_STRING("(" << *entity << " store:" << entity->GetStoreId() 
                                 << " id:" << entity_id_ << ")");
   std::string offsetNodeName = BUILD_STRING(node_name_ << " offset");

   totalObjectCount_++;
   node_ = h3dAddGroupNode(parent, node_name_.c_str());
   offsetNode_ = h3dAddVoxelModelNode(node_, offsetNodeName.c_str());

   // xxx: convert to something more dm::Trace like...
   selection_guard_ = Renderer::GetInstance().SetSelectionForNode(node_, entity);

   query_flags_ = 0;
}

bool RenderEntity::IsValid() const
{
   return initialized_ && !destroyed_;
}

void RenderEntity::FinishConstruction()
{
   nodeToRenderEntity[node_] = shared_from_this();

   auto entity = GetEntity();
   if (entity) {
      components_trace_ = entity->TraceComponents("RenderEntity c++ components", dm::RENDER_TRACES)
                                       ->OnAdded([this](core::StaticString name, std::shared_ptr<dm::Object> obj) {
                                          AddComponent(name, obj);
                                       })
                                       ->OnRemoved([this](core::StaticString name) {
                                          RemoveComponent(name);
                                       })
                                       ->PushObjectState();

      lua_components_trace_ = entity->TraceLuaComponents("RenderEntity lua components", dm::RENDER_TRACES)
                                       ->OnAdded([this](core::StaticString name, om::DataStorePtr obj) {
                                          AddLuaComponent(name, obj);
                                       })
                                       ->OnRemoved([this](core::StaticString name) {
                                          RemoveComponent(name);
                                       })
                                       ->PushObjectState();
   }

   // Make sure we always have a RenderRenderInfo component, since that's where things like material
   // override and visible override get resolved.  If someone on the server later adds a render_info
   // component, this copy will get blown away (which is just fine!).
   auto i = components_.find("render_info");
   if (i == components_.end()) {
      components_["render_info"] = std::make_shared<RenderRenderInfo>(*this, nullptr);
   }

   UpdateInvariantRenderers();
   initialized_ = true;
}

RenderEntity::~RenderEntity()
{
   E_LOG(7) << "destroying render entity " << node_name_;

   if (!destroyed_) {
      Destroy();
   }
}

void RenderEntity::Destroy()
{
   destroyed_ = true;

   nodeToRenderEntity.erase(node_);

   lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();

   if (!script->IsShutDown()) {
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
   }
   lua_invariants_.clear();
   components_.clear();

   h3dRemoveNode(offsetNode_);
   h3dRemoveNode(node_);
   totalObjectCount_--;

   offsetNode_ = 0;
   node_ = 0;
}

void RenderEntity::SetParent(H3DNode parent)
{
   if (!parent) {
      parent = RenderNode::GetUnparentedNode();
   }
   //TODO: what happens when the node_ is destroyed before this happens? 
   //Currently, crash (see promotion_test.lua). Fix before the Wildfire Mod
   h3dSetNodeParent(node_, parent);
}

/*
 * -- RenderEntity::SetParentOverride
 *
 * Specifies whether or not the parent for this entity should be determined
 * by the object hierarchy in the object model or manually specified by lua.
 * Ideally, all objects would obey the object hierarchy.  In some cases, however,
 * we need to explicitly disable it.  Consider the little ghost entities attached
 * to cursors when moving items.  Those need to be created on the Horde root
 * node and moved around by some input listener.  We can't put them in the root
 * node of the object model, since it's read-only on the client side.  In
 * practice, this should be called in exactly one place, which is the manual
 * creation of render entities on entities created in the authoring store.
 *
 */
void RenderEntity::SetParentOverride(bool enabled)
{
   parentOverride_ = enabled;
}

bool RenderEntity::GetParentOverride() const
{
   return parentOverride_;
}


H3DNode RenderEntity::GetParent() const
{
   return h3dGetNodeParent(node_);
}

/*
 * -- RenderEntity::GetNode
 *
 * Returns the horde node positions exactly where the entity is rendered.
 * This is offset om::Mob::render_offset units from the origin node.  If you
 * want to render something in the visual-coordinate-system (is that thing?)
 * of the Entity, you want to use this node as a reference.
 *
 */
H3DNode RenderEntity::GetNode() const
{
   return offsetNode_;
}


/*
 * -- RenderEntity::GetOriginNode
 *
 * Returns the horde node placed exactly where the om::Mob component specifies
 * the Entity's location is.  This may not be exacly where the entity is
 * drawn on screen if the om::Mob's render offset value is not Point3f::zero.
 * If you want to do something with the node (e.g. add your own renderables
 * under it), this is probably not the node you want.  See :GetNode()
 *
 */
H3DNode RenderEntity::GetOriginNode() const
{
   return node_;
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
                           render_invariant = ctor(re, script->JsonToLua(entry));
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

std::shared_ptr<RenderComponent> RenderEntity::GetComponentRenderer(core::StaticString name) const
{
   auto i = components_.find(name);
   if (i != components_.end()) {
      return i->second;
   }
   return nullptr;
}

void RenderEntity::AddComponent(core::StaticString name, std::shared_ptr<dm::Object> value)
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

void RenderEntity::AddLuaComponent(core::StaticString name, om::DataStorePtr obj)
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


void RenderEntity::RemoveComponent(core::StaticString name)
{
   components_.erase(name);
}

void RenderEntity::SetSelected(bool selected)
{
   h3dTwiddleNodeFlags(node_, H3DNodeFlags::Selected, selected, true);
}

dm::ObjectId RenderEntity::GetObjectId() const
{
   return entity_id_;
}

void RenderEntity::SetRenderInfoDirtyBits(int bits)
{
   if (!destroyed_) {
      auto i = components_.find("render_info");
      if (i != components_.end()) {
         std::shared_ptr<RenderRenderInfo> ri = std::static_pointer_cast<RenderRenderInfo>(i->second);
         ri->SetDirtyBits(bits);
      }
   }
}

void RenderEntity::SetModelVariantOverride(std::string const& variant)
{
   model_variant_override_ = variant;
   SetRenderInfoDirtyBits(RenderRenderInfo::MODEL_DIRTY);
}

// This should probably operate like a stack where callers can push and pop a material override onto the stack.
void RenderEntity::SetMaterialOverride(std::string const& material)
{
   material_override_ = material;
   SetRenderInfoDirtyBits(RenderRenderInfo::MATERIAL_DIRTY);
}

std::string RenderEntity::GetMaterialPathFromKind(std::string const& matKind, std::string const& deflt) const
{
   std::string matPath;
   auto entity = entity_.lock();

   if (entity) {
      auto lookupCallback = [&matKind, &matPath, &deflt](JSONNode const& data) {
         json::Node n(data);
         matPath = n.get("entity_data.stonehearth:render_materials." + matKind, deflt);
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

void RenderEntity::ForAllSceneNodes(std::function<void(H3DNode node)> const& fn)
{
   ForAllSceneNodes(node_, fn);
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
void RenderEntity::ForAllSceneNodes(H3DNode node, std::function<void(H3DNode node)> const& fn)
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

// Returns true if all visibility requests are true.
bool RenderEntity::GetVisibleOverride() const
{
   return visible_override_ref_count_ == 0;
}

// The visible_override_ref_count_ indicates the number of outstanding requests to hide the entity.
// Consider renaming the method to make it clear what the behavior is.
void RenderEntity::SetVisibleOverride(bool visible)
{
   if (visible) {
      if (visible_override_ref_count_ > 0) {
         visible_override_ref_count_--;
      }
   } else {
      if (visible_override_ref_count_ < UINT32_MAX) {
         visible_override_ref_count_++;
      }
   }
   if (visible_override_ref_count_ == 0 || visible_override_ref_count_ == 1) {
      SetRenderInfoDirtyBits(RenderRenderInfo::VISIBLE_DIRTY);
   }
}

// Returns a handle used to request visibility changes to the entity.
// The entity is visible if all requestors set visible to true
RenderEntity::VisibilityHandlePtr RenderEntity::GetVisibilityOverrideHandle()
{
   return std::make_shared<VisibilityHandle>(shared_from_this());
}

RenderEntity::VisibilityHandle::VisibilityHandle(RenderEntityRef renderEntityRef) :
   renderEntityRef_(renderEntityRef),
   visible_(true)
{
}

RenderEntity::VisibilityHandle::~VisibilityHandle()
{
   Destroy();
}

void RenderEntity::VisibilityHandle::SetVisible(bool visible)
{
   if (visible != visible_) {
      visible_ = visible;
      RenderEntityPtr renderEntityPtr = renderEntityRef_.lock();
      if (renderEntityPtr) {
         renderEntityPtr->SetVisibleOverride(visible);
      }
   }
}

bool RenderEntity::VisibilityHandle::GetVisible()
{
   return visible_;
}

void RenderEntity::VisibilityHandle::Destroy()
{
   SetVisible(true);
}

void RenderEntity::Pose(const char* animationName, int time)
{
   res::AnimationPtr animation = GetAnimation(animationName);
   if (animation) {
      Pose(animation, time);
   }
}

void RenderEntity::Pose(res::AnimationPtr const& animation, int offset)
{
   float scale = skeleton_.GetScale();
   animation->MoveNodes(offset, scale, [this, scale](std::string const& bone, const csg::Transform &transform) {
      H3DNode node = skeleton_.GetSceneNode(bone);
      if (node) {
         E_LOG(9) << "moving " << bone << " to " << transform << "(node: " << node << " scale:" << scale << ")";
         MoveSceneNode(node, transform);
      }
   });
}

res::AnimationPtr RenderEntity::GetAnimation(const char* animationName)
{
   om::EntityPtr entity = entity_.lock();
   if (!entity) {
      return nullptr;
   }
   om::RenderInfoPtr renderInfo = entity->GetComponent<om::RenderInfo>();
   if (!renderInfo) {
      return nullptr;
   }

   // compute the location of the animation
   std::string animationTable = renderInfo->GetAnimationTable();
   std::string animationRoot;
   res::ResourceManager2::GetInstance().LookupJson(animationTable, [&](const json::Node& json) {
      animationRoot = json.get<std::string>("animation_root", "");
   });

   std::string path = animationRoot + "/" + animationName;
   return res::ResourceManager2::GetInstance().LookupAnimation(path);
}
