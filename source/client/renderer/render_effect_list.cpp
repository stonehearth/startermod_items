#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <claw/tween/easing/easing_back.hpp>
#include <claw/tween/easing/easing_bounce.hpp>
#include <claw/tween/easing/easing_circ.hpp>
#include <claw/tween/easing/easing_cubic.hpp>
#include <claw/tween/easing/easing_elastic.hpp>
#include <claw/tween/easing/easing_linear.hpp>
#include <claw/tween/easing/easing_none.hpp>
#include <claw/tween/easing/easing_quad.hpp>
#include <claw/tween/easing/easing_quart.hpp>
#include <claw/tween/easing/easing_quint.hpp>
#include <claw/tween/easing/easing_sine.hpp>
#include <regex>
#include "renderer.h"
#include "render_entity.h"
#include "render_effect_list.h"
#include "skeleton.h"
#include "client/client.h"
#include "dm/map_trace.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/components/mob.ridl.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "lib/audio/audio_manager.h"
#include "lib/json/node.h"
#include "lib/lua/script_host.h"
#include "lib/perfmon/perfmon.h"
#include "lib/audio/input_stream.h"
#include "csg/random_number_generator.h"
#include <SFML/Audio.hpp>
#include "horde3d\Source\Horde3DEngine\egHudElement.h"
#include "horde3d\Source\Shared\utMath.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define EL_LOG_NOPREFIX(level)   LOG(renderer.effects_list, level)
#define EL_LOG(level)            LOG_CATEGORY(renderer.effects_list, level, log_prefix_)

int GetStartTime(const JSONNode& node) 
{
   auto i = node.find("start_time");
   if (i != node.end()) {
      return i->as_int();
   }
   i = node.find("start_frame");
   if (i != node.end()) {
      return (int)std::floor(i->as_int() * 1000.0f / 30.0f);
   }
   return 0;
}

static void MoveSceneNode(H3DNode node, const csg::Transform& t, float scale)
{
   csg::Matrix4 m(t.orientation);
   
   m[12] = t.position.x * scale;
   m[13] = t.position.y * scale;
   m[14] = t.position.z * scale;
   
   bool result = h3dSetNodeTransMat(node, m.get_float_ptr());
   if (!result) {
      EL_LOG_NOPREFIX(5) << "failed to set transform on node " << node << ".";
   }
}

RenderEffectList::RenderEffectList(RenderEntity& entity, om::EffectListPtr effectList) :
   entity_(entity),
   effectList_(effectList)
{
   ASSERT(effectList);

   log_prefix_ = BUILD_STRING("[" << *entity.GetEntity() << " effect_list" << "]");

   effects_list_trace_ = \
      effectList->TraceEffects("render", dm::RENDER_TRACES)
                     ->OnUpdated([this](om::EffectList::EffectChangeMap const& added,
                                        std::vector<int> const& removed) {
                        for (auto const& entry : added) {
                           AddEffect(entry.first, entry.second);
                        }
                        for (int effect_id : removed) {
                           RemoveEffect(effect_id);
                        }
                     })
                     ->PushObjectState();

   renderer_guard_ += Renderer::GetInstance().OnRenderFrameStart([this](FrameStartInfo const& info) {
      perfmon::TimelineCounterGuard tcg("update effects");
      UpdateEffects(info);
   });
}

RenderEffectList::~RenderEffectList()
{
   EL_LOG_NOPREFIX(9) << "destroying render effect list";
}

void RenderEffectList::AddEffect(int effect_id, const om::EffectPtr effect)
{
   std::string name = effect->GetName();
   effects_[effect_id] = RenderInnerEffectList(entity_, effect);
}

void RenderEffectList::RemoveEffect(int effect_id)
{
   effects_.erase(effect_id);
}

void RenderEffectList::UpdateEffects(FrameStartInfo const& info)
{
   if (!entity_.GetEntity()) {
      EL_LOG(5) << "skipping updated effects for destroyed entity";
      return;
   }
   EL_LOG(5) << "updating effects for entity " << entity_.GetName();

   auto i = effects_.begin();
   while (i != effects_.end()) {
      bool finished = false;
      i->second.Update(info, finished);
      if (finished) {
         i = effects_.erase(i);
      } else {
         i++;
      }
   }
}

RenderInnerEffectList::RenderInnerEffectList(RenderEntity& renderEntity, om::EffectPtr effect)
{
   log_prefix_ = BUILD_STRING("[" << *renderEntity.GetEntity() << " inner_effect_list" << "]");
   try {
      std::string name = effect->GetName();
      res::ResourceManager2::GetInstance().LookupJson(name, [&](const JSONNode& data) {
         for (const JSONNode& node : data["tracks"]) {
            std::string type = node["type"].as_string();
            std::shared_ptr<RenderEffect> e;
            if (type == "animation_effect") {
               e = std::make_shared<RenderAnimationEffect>(renderEntity, effect, node); 
            } else if (type == "attach_item_effect") {
               e = std::make_shared<RenderAttachItemEffect>(renderEntity, effect, node); 
            } else if (type == "floating_combat_text") {
               e = std::make_shared<FloatingCombatTextEffect>(renderEntity, effect, node); 
            } else if (type == "hide_bone") {
               e = std::make_shared<HideBoneEffect>(renderEntity, effect, node);
            } else if (type == "activity_overlay_effect") {
               e = std::make_shared<ActivityOverlayEffect>(renderEntity, effect, node);
            } else if (type == "unit_status_effect") {
               e = std::make_shared<UnitStatusEffect>(renderEntity, effect, node);
            } else if (type == "sound_effect") {
               e = std::make_shared<PlaySoundEffect>(renderEntity, effect, node); 
            } else if (type == "cubemitter") {
               e = std::make_shared<CubemitterEffect>(renderEntity, effect, node);
            } else if (type == "light") {
               e = std::make_shared<LightEffect>(renderEntity, effect, node);
            }
            if (e) {
               effects_.push_back(e);
            }
         }
      });
   } catch (std::exception& e) {
      EL_LOG(5) << "failed to create effect: " << e.what();
   }
}

RenderInnerEffectList::~RenderInnerEffectList()
{
   EL_LOG_NOPREFIX(9) << "destroying render inner effect list";
}

void RenderInnerEffectList::Update(FrameStartInfo const& info, bool& finished)
{
   int i = 0, c = effects_.size();
   while (i < c) {
      auto effect = effects_[i];
      bool done = false;
      effect->Update(info, done);
      if (done) {
         effects_[i] = effects_[--c];
         effects_.resize(c);
         // Keep the effect around until the effect has run it's entire course.
         // The ensures that destructors of each individual effect don't run
         // until all effects are finished.  For example, if an effect has overridden
         // flags on an render object, those flags need to stay in effect until all
         // effects in the list have finished
         finished_.push_back(effect);
      } else {
         i++;
      }
   }
   finished = effects_.empty();
   if (finished) {
      finished_.clear();
   }
}


RenderEffect::RenderEffect(RenderEntity& entity, std::string const& prefix) :
   entity_(entity)
{
   log_prefix_ = BUILD_STRING("[" << *entity_.GetEntity() << " " << prefix << "]");
}

///////////////////////////////////////////////////////////////////////////////
// RenderAnimationEffect
///////////////////////////////////////////////////////////////////////////////

RenderAnimationEffect::RenderAnimationEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "animation")
{
   om::EntityPtr entity = e.GetEntity();

   if (entity) {
      int now = effect->GetStartTime();
      animationName_ = node["animation"].as_string();
   
      // compute the location of the animation
      std::string animationTable = entity->GetComponent<om::RenderInfo>()->GetAnimationTable();
      std::string animationRoot;
      res::ResourceManager2::GetInstance().LookupJson(animationTable, [&](const json::Node& json) {
         animationRoot = json.get<std::string>("animation_root", "");
      });

      animationName_ = animationRoot + "/" + animationName_;
      animation_ = res::ResourceManager2::GetInstance().LookupAnimation(animationName_);

      if (animation_) {
         speed_ = std::max(json::Node(node).get<float>("speed", 1.0f), 0.01f);

         startTime_ = GetStartTime(node) + now;
         auto i = node.find("loop");
         if (i != node.end() && i->as_bool()) {
            duration_ = 0;
         } else {
            auto i = node.find("end_time");
            if (i != node.end()) {
               duration_ = (float)i->as_float();
            } else {
               duration_ = animation_->GetDuration();
            }
         }
         duration_ /= speed_;
      }
      EL_LOG(5) << "starting animation effect" << animationName_ << "(start_time: " << startTime_ << ")";
   }
}

RenderAnimationEffect::~RenderAnimationEffect()
{
   EL_LOG(9) << "destroying animation effect" << animationName_;
}

void RenderAnimationEffect::Update(FrameStartInfo const& info, bool& finished)
{
   EL_LOG(9) << "updating animation effect" << animationName_;
   if (!animation_) {
      EL_LOG(9) << "marking animation effect " << animationName_ << " finished (no animation)";
      finished = true;
      return;
   }
   int offset = static_cast<int>((info.now - startTime_) * speed_);
   if (duration_) {    
      if (info.now > startTime_ + duration_) {
         EL_LOG(9) << "marking animation effect " << animationName_ << " finished (duration exceeded)";
         finished = true;
      }

      // if we're not looping, make sure we stop at the end frame.
      int animationDuration = (int)animation_->GetDuration();
      if (offset >= animationDuration) {
         offset = animationDuration - 1;
      }
   }

   animation_->MoveNodes(offset, [&](std::string const& bone, const csg::Transform &transform) {
      H3DNode node = entity_.GetSkeleton().GetSceneNode(bone);
      if (node) {
         MoveSceneNode(node, transform, entity_.GetSkeleton().GetScale());
      }
   });
}

///////////////////////////////////////////////////////////////////////////////
// HideBoneEffect
///////////////////////////////////////////////////////////////////////////////

HideBoneEffect::HideBoneEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "hide bone"),
   boneNode_(0),
   boneNodeFlags_(0)
{
   auto i = node.find("bone");
   if (i != node.end()) {
      boneNode_ = entity_.GetSkeleton().GetSceneNode(i->as_string());
      boneNodeFlags_ = h3dGetNodeFlags(boneNode_);
   }
}

HideBoneEffect::~HideBoneEffect()
{
   if (boneNode_ ) {
      h3dSetNodeFlags(boneNode_, boneNodeFlags_, false);  
   }
}

void HideBoneEffect::Update(FrameStartInfo const& info, bool& finished)
{
   if (boneNode_) {      
      h3dSetNodeFlags(boneNode_, boneNodeFlags_ | H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, false);
   }
   finished = true;
}

///////////////////////////////////////////////////////////////////////////////
// CubemitterEffect
///////////////////////////////////////////////////////////////////////////////

CubemitterEffect::CubemitterEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "cubemitter"),
   cubemitterNode_(0),
   parent_(e.GetNode())
{
   json::Node o(node);

   int now = effect->GetStartTime();
   startTime_ = o.get("start_time", 0) + now;
   endTime_ = o.get("end_time", 0);
   if (endTime_) {
      endTime_ += now;
   }
   filename_ = node["cubemitter"].as_string();

   json::Node transforms = o.get_node("transforms");
   pos_.x = transforms.get("x", 0.0f);
   pos_.y = transforms.get("y", 0.0f);
   pos_.z = transforms.get("z", 0.0f);
   rot_.x = transforms.get("rx", 0.0f);
   rot_.y = transforms.get("ry", 0.0f);
   rot_.z = transforms.get("rz", 0.0f);
}

CubemitterEffect::~CubemitterEffect()
{
}

void CubemitterEffect::Update(FrameStartInfo const& info, bool& finished)
{
   if (info.now > startTime_) {
      if (!cubemitterNode_.get()) {
         H3DRes cubeRes = h3dAddResource(RT_CubemitterResource, filename_.c_str(), 0);
         H3DNode c = h3dRadiantAddCubemitterNode(parent_, "cu", cubeRes);
         cubemitterNode_ = H3DCubemitterNodeUnique(c);

         h3dSetNodeTransform(cubemitterNode_.get(), pos_.x, pos_.y, pos_.z, rot_.x, rot_.y, rot_.z, 1, 1, 1);
         finished = false;
         return;
      }
   }
   finished = (endTime_ > 0) && (info.now > endTime_);
   if (finished) {
      cubemitterNode_.reset(0);
   }
}

///////////////////////////////////////////////////////////////////////////////
// LightEffect
///////////////////////////////////////////////////////////////////////////////

LightEffect::LightEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "light effect"),
   lightNode_(0)
{
   auto animatedLightFileName = node["light"].as_string();
   // TODO: for just a moment, hardcode some light values.
   H3DRes lightRes = h3dAddResource(RT_AnimatedLightResource, animatedLightFileName.c_str(), 0);
   H3DNode l = h3dRadiantAddAnimatedLightNode(e.GetNode(), "ln", lightRes);

   float x, y, z;

   parseTransforms(node["transforms"], &x, &y, &z);
   h3dSetNodeTransform(l, x, y, z, 0, 0, 0, 1, 1, 1);

   lightNode_ = H3DNodeUnique(l);
}

void LightEffect::parseTransforms(const JSONNode& node, float* x, float* y, float* z)
{
   json::Node o(node);

   *x = o.get("x", 0.0f);
   *y = o.get("y", 0.0f);
   *z = o.get("z", 0.0f);
}


LightEffect::~LightEffect()
{
}

void LightEffect::Update(FrameStartInfo const& info, bool& finished)
{
   finished = false;
}


///////////////////////////////////////////////////////////////////////////////
// ActivityOverlayEffect
///////////////////////////////////////////////////////////////////////////////

void getBoundsForGroupNode(float* minX, float* maxX, float *minY, float *maxY, H3DNode node)
{
   float sMinY = 999999, sMaxY = -999999;
   float sMinX = 999999, sMaxX = -999999;
   *minY = sMinY;
   *maxY = sMaxY;
   *minX = sMinX;
   *maxX = sMaxX;

   int i = 0;
   H3DNode n = 0;
   while ((n = h3dGetNodeChild(node, i)) != 0) {
      if (h3dGetNodeType(n) == Horde3D::SceneNodeTypes::VoxelModel) {
         h3dGetNodeAABB(n, &sMinX, &sMinY, nullptr, &sMaxX, &sMaxY, nullptr);
      } else if (h3dGetNodeType(n) == Horde3D::SceneNodeTypes::Group) {
         getBoundsForGroupNode(&sMinX, &sMaxX, &sMinY, &sMaxY, n);
      }
      if (*minY > sMinY) {
         *minY = sMinY;
      }
      if (*maxY < sMaxY) {
         *maxY = sMaxY;
      }
      if (*minX > sMinX) {
         *minX = sMinX;
      }
      if (*maxX < sMaxX) {
         *maxX = sMaxX;
      }
      i++;
   }
}

ActivityOverlayEffect::ActivityOverlayEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "activity overlay"),
   _positioned(false)
{
   json::Node cjo(node);

   _material = cjo.get("material", std::string("materials/chop_overlay/chop_overlay.material.xml"));
   _overlayWidth = cjo.get("width", 64);
   _overlayHeight = cjo.get("height", 64);
   _yOffset = cjo.get("y_offset", 0);
   _hud = h3dAddHudElementNode(e.GetNode(), "");
   overlayNode_ = H3DNodeUnique(_hud->getHandle());
}

ActivityOverlayEffect::~ActivityOverlayEffect()
{
   h3dRemoveResource(_matRes);
}

bool ActivityOverlayEffect::PositionOverlayNode()
{
   float minX, maxX, minY, maxY;
   getBoundsForGroupNode(&minX, &maxX, &minY, &maxY, entity_.GetNode());
   if (minX > maxX || minY > maxY) {
      EL_LOG(8) << "could not compute bounds of render entity node";
      return false;
   }
   EL_LOG(8) << "render entity node bounds are " << minX << ", " << minY << ", " << maxX << ", " << maxY;

   h3dSetNodeTransform(_hud->getHandle(), 0, maxY - minY + 4, 0, 0, 0, 0, 1, 1, 1);

   // Because there may be animated textures associated with the material, we clone the resource
   // so that all related nodes can animate at their own pace.
   // This is basically a hack until I put in a material_instance/material_template distinction.
   H3DRes mat = h3dAddResource(H3DResTypes::Material, _material.c_str(), 0);
   // Can't clone until we load!
   Renderer::GetInstance().LoadResources();
   _matRes = h3dCloneResource(mat, "");
   _hud->addScreenspaceRect(_overlayWidth, _overlayHeight, (int)(-_overlayWidth / 2.0f), _yOffset, Horde3D::Vec4f(1, 1, 1, 1), _matRes);
   return true;
}

void ActivityOverlayEffect::Update(FrameStartInfo const& info, bool& finished)
{
   finished = false;
   if (!_positioned) {
      PositionOverlayNode();
      _positioned = true;
   }
}


///////////////////////////////////////////////////////////////////////////////
// UnitStatusEffect
///////////////////////////////////////////////////////////////////////////////

UnitStatusEffect::UnitStatusEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "unit status")
{
   json::Node cjo(node);
   std::string matName = cjo.get("material", std::string("materials/sleepy_indicator/sleepy_indicator.material.xml"));
   float statusWidth = cjo.get("width", 3.0f);
   float statusHeight = cjo.get("height", 3.0f);
   float xOffset = cjo.get("xOffset", 0.0f);
   float yOffset = cjo.get("yOffset", 0.0f);
   H3DNode n = e.GetSkeleton().GetSceneNode(cjo.get("bone", std::string("head")));

   H3DRes mat = h3dAddResource(H3DResTypes::Material, matName.c_str(), 0);

   Horde3D::HudElementNode* hud = h3dAddHudElementNode(n, "");
   h3dSetNodeTransform(hud->getHandle(), 0, 0, 0, 0, 0, 0, 1, 1, 1);
   hud->addWorldspaceRect(statusWidth, statusHeight, 
      xOffset- (statusWidth / 2.0f), yOffset, Horde3D::Vec4f(1, 1, 1, 1), mat);
   statusNode_ = H3DNodeUnique(hud->getHandle());
}

UnitStatusEffect::~UnitStatusEffect()
{
}

void UnitStatusEffect::Update(FrameStartInfo const& info, bool& finished)
{
   finished = false;
}

///////////////////////////////////////////////////////////////////////////////
// RenderAttachItemEffect
///////////////////////////////////////////////////////////////////////////////

RenderAttachItemEffect::RenderAttachItemEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "attach item"),
   finished_(false),
   use_model_variant_override_(false)
{
   std::string kind = node["item"].as_string();
   om::EntityPtr item;
   int now = effect->GetStartTime();

   if (boost::starts_with(kind, "{{") && boost::ends_with(kind, "}}")) {
      std::string arg(kind.begin() + 2, kind.end() - 2);
      om::Selection selection = effect->GetParam(arg);
      if (selection.HasEntities()) {
         dm::ObjectId id = selection.GetEntities().front();
         item = Client::GetInstance().GetStore().FetchObject<om::Entity>(id);
#if 0
         // this is illegal.  if we really need to move it, move the render entity!
         if (item) {
            auto mob = item->GetComponent<om::Mob>();
            if (mob) {
               mob->MoveTo(csg::Point3f(0, 0, 0));
               mob->TurnToAngle(0);
            }
         }
#endif
      }
   } else {
      item = authored_entity_ = Client::GetInstance().GetAuthoringStore().AllocObject<om::Entity>();
      try {
         om::Stonehearth::InitEntity(item, kind, Client::GetInstance().GetScriptHost()->GetInterpreter());
      } catch (res::Exception &e) {
         // xxx: put this in the error browser!!
         EL_LOG(5) << "!!!!!!! ERROR IN RenderAttachItemEffect: " << e.what();
      }
   }

   if (item) {
      startTime_ = GetStartTime(node) + now;
      bone_ = node["bone"].as_string();

      om::MobPtr mob = item->GetComponent<om::Mob>();
      if (mob) {
         mob->SetLocationGridAligned(csg::Point3::zero);
      }

      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone_);
      render_item_ = Renderer::GetInstance().CreateRenderEntity(parent, item);
      render_item_->SetParent(0);

      auto i = node.find("render_info");
      if (i != node.end()) {
         // xxx: can we get rid of this abomination? -- tony
         auto j = i->find("model_variant");
         if (j != i->end()) {
            model_variant_override_ = j->as_string();
            use_model_variant_override_ = true;
         }
      }
   }
}

RenderAttachItemEffect::~RenderAttachItemEffect()
{
   if (use_model_variant_override_) {
      use_model_variant_override_ = false;
      render_item_->SetModelVariantOverride("");
   }
}

void RenderAttachItemEffect::Update(FrameStartInfo const& info, bool& finished)
{
   if (finished_) {
      finished = true;
      return;
   }

   if (info.now >= startTime_) {
      finished = true;
      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone_);
      render_item_->SetParent(parent);
      if (use_model_variant_override_) {
         render_item_->SetModelVariantOverride(model_variant_override_);
      }
      finished_ = true;
   }
}

// see http://libclaw.sourceforge.net/tweeners.html

claw::tween::single_tweener::easing_function
get_easing_function(std::string easing)
{
#define GET_EASING_FUNCTION(name) \
   if (eq == std::string(#name)) { \
      if (method == "in") { return claw::tween::easing_ ## name::ease_in; } \
      if (method == "out") { return claw::tween::easing_ ## name::ease_out; } \
      if (method == "in_out") { return claw::tween::easing_ ## name::ease_in_out; } \
   }

   int offset = easing.find('_');
   if (offset != std::string::npos) {
      std::string eq = easing.substr(0, offset);
      std::string method = easing.substr(offset + 1);

      GET_EASING_FUNCTION(back)
      GET_EASING_FUNCTION(bounce)
      GET_EASING_FUNCTION(circ)
      GET_EASING_FUNCTION(cubic)
      GET_EASING_FUNCTION(elastic)
      GET_EASING_FUNCTION(linear)
      GET_EASING_FUNCTION(none)
      GET_EASING_FUNCTION(quad)
      GET_EASING_FUNCTION(quart)
      GET_EASING_FUNCTION(quint)
      GET_EASING_FUNCTION(sine)
   }
   return claw::tween::easing_linear::ease_in;
#undef GET_EASING_FUNCTION
}

///////////////////////////////////////////////////////////////////////////////
// FloatingCombatTextEffect
///////////////////////////////////////////////////////////////////////////////

FloatingCombatTextEffect::FloatingCombatTextEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "floating combat text"),
   height_(0),
   lastUpdated_(0)
{
   startTime_  = effect->GetStartTime();
   std::string text = node["text"].as_string();

   // replace the text...
   std::regex mustache("\\{\\{([^\\}]+)\\}\\}");
   std::smatch match;
   while (std::regex_search(text, match, mustache)) {
      std::string key = match[1].str();
      std::string value = effect->GetParam(key).GetString();
      text = match.prefix().str() + value + match.suffix().str();
   }
   om::EntityPtr entity = Client::GetInstance().GetStore().FetchObject<om::Entity>(e.GetObjectId());
   if (entity) {
      om::RenderInfoPtr render_info = entity->GetComponent<om::RenderInfo>();
      if (render_info) {
         std::string animationTableName = render_info->GetAnimationTable();

         json::Node cs;
         res::ResourceManager2::GetInstance().LookupJson(animationTableName, [&](const json::Node& json) {
            cs = json.get<JSONNode>("collision_shape", JSONNode());
         });
         height_ = cs.get<float>("height", 4.0f);
         height_ *= 0.1f; // xxx - take this out of the same place where we store the face that the model is 10x too big
      }
   }
   int distance = node["distance"].as_int();
   int duration = node["duration"].as_int();
   double end_height = height_ + distance;
   std::string easing = node["distance_easing"].as_string();

   tweener_.reset(new claw::tween::single_tweener(height_, end_height, duration, get_easing_function(easing)));

   toastNode_ = h3dRadiantCreateToastNode(entity_.GetNode(), "floating combat text " + stdutil::ToString(effect->GetObjectId()));
   if (toastNode_) {
      float scale = 0.03f;
      toastNode_->SetText(text.c_str());
   }
   Renderer::GetInstance().LoadResources();
}

FloatingCombatTextEffect::~FloatingCombatTextEffect()
{
   if (toastNode_) {
      h3dRemoveNode(toastNode_->GetNode());
      toastNode_ = nullptr;
   }
}

void FloatingCombatTextEffect::Update(FrameStartInfo const& info, bool& finished)
{
   double dt = lastUpdated_ ? (info.now - lastUpdated_) : 0.0;
   lastUpdated_ = info.now;

   if (tweener_->is_finished()) {
      if (toastNode_) {
         h3dRemoveNode(toastNode_->GetNode());
         toastNode_ = nullptr;
      }
      finished = true;
      return;
   }
   tweener_->update(dt);
   h3dSetNodeTransform(toastNode_->GetNode(), 0, static_cast<float>(height_), 0, 0, 0, 0, 1, 1, 1);
}

/* PlaySoundEffect
 * Class to play a short sound. Loads sound into memory.
 * TODO: if we run out of sounds too quickly, only create sounds if the
 * camera is in range of them. (Make sound a ptr and create/dispose as necessary)
**/

/* PlaySoundEffect
 * Constructor
 * When we have a new effect, create an sf::SoundBuffer and sf::Sound to play it
 * Store all relevant parameters
*/
#define PLAY_SOUND_EFFECT_DEFAULT_START_DELAY 0;
#define PLAY_SOUND_EFFECT_DEFAULT_MIN_DIST 20;
#define PLAY_SOUND_EFFECT_DEFAULT_MAX_DIST 60;
#define PLAY_SOUND_EFFECT_DEFAULT_LOOP false;
#define PLAY_SOUND_EFFECT_MIN_VOLUME 0.1f;

PlaySoundEffect::PlaySoundEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   RenderEffect(e, "sound")
{
   startTime_ = effect->GetStartTime();
   firstPlay_ = true;

   std::string track = "";
   json::Node n(node["track"]);
   if (n.type() == JSON_STRING) {
      track = n.get_internal_node().as_string();
   } else if (n.type() == JSON_NODE) {
      if (n.get<std::string>("type", "") == "one_of") {
         JSONNode items = n.get("items", JSONNode());
         csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();
         uint c = rng.GetInt<uint>(0, items.size() - 1);
         ASSERT(c < items.size());
         track = items.at(c).as_string();
      }
   }
   
   sound_ = audio::AudioManager::GetInstance().CreateSound(track);
   AssignFromJSON_(node);
   if (delay_ == 0) {
      //TODO: if camera is farther than maxDistance, create sound and play
	   sound_->play();
   }
}

/* PlaySoundEffect::AssignFromJSON
 * Make initial values for all the sound parameters
 * Extract the optional values from the json, verify, and assign them.
*/
void PlaySoundEffect::AssignFromJSON_(const JSONNode& node) {
   bool loop = PLAY_SOUND_EFFECT_DEFAULT_LOOP;
   int delay = PLAY_SOUND_EFFECT_DEFAULT_START_DELAY;
   int minDistance = PLAY_SOUND_EFFECT_DEFAULT_MIN_DIST; 
   int maxDistance = PLAY_SOUND_EFFECT_DEFAULT_MAX_DIST;
   float attenuation;

   //Optional members
   auto i = node.find("start_delay");
   if (i != node.end()) {
      delay = (int)i->as_int();
   } 

   i = node.find("min_distance");
   if (i != node.end()) {
      minDistance = (int)i->as_int();
      if (minDistance < 1) {
         minDistance = 1;
      }
   }

   i = node.find("max_distance");
   if (i != node.end()) {
      maxDistance = (int)i->as_int();
      if (maxDistance <= minDistance) {
         maxDistance = minDistance + 1;
      }
   }

   i = node.find("loop");
   if (i != node.end()) {
      loop = (int)i->as_bool();
   } 

   i = node.find("volume");
   if (i != node.end()) {
      double value = std::min(std::max(i->as_float(), 0.0), 100.0);
      sound_->setVolume(static_cast<float>(value));
   }

   i = node.find("pitch");
   if (i != node.end()) {
      sound_->setPitch(static_cast<float>(i->as_float()));
   }

   attenuation = CalculateAttenuation(maxDistance, minDistance);

   //Set member variables
   delay_ = delay;
   maxDistance_ = maxDistance; 

   //Set sound parameters
   sound_->setLoop(loop);
   sound_->setAttenuation(attenuation);
   sound_->setMinDistance((float)minDistance);
}

/* PlaySoundEffect::CalculateAttenuation
 *  SFML has an attentuation parameter that determines how fast sound fades.
 * Since we'd rather expose max distance to the user (the max distance at which a sound can be heard)
 * we will use sfml's attentuation function to figure out what attentuation should be based on
 * max distance. 
 * Assumes MaxDistance >= MinDistance + 1
 * Equation: A = (maxDistance_/(.01) - minDistance_)/(maxDistance_ - minDistance_)
 * Note: remember, doing an operation of int and float converts everything to a float
 * Returns a float. 1 means the sound sticks around for a long time. Higher numbers means it softens faster.
*/
float PlaySoundEffect::CalculateAttenuation(int maxDistance, int minDistance) {
   float interm1 = maxDistance / PLAY_SOUND_EFFECT_MIN_VOLUME;
   float interm2 = interm1 - minDistance;
   float interm3 = (float)(maxDistance - minDistance);
   return (interm2/interm3);
}

PlaySoundEffect::~PlaySoundEffect()
{
   sound_->stop();
}

/* PlaySoundEffect::Update
 * Update location of sound.
 * If we were delaying the sound start and now is the time to actually start the sound, start it. 
 * If the sound was in a loop, we are not yet finished.
 * If the sound is finished, set finished to true
 * TODO: on update, check if camera is between min and max distances. If so, create/destroy sound as necessary
**/
void PlaySoundEffect::Update(FrameStartInfo const& info, bool& finished)
{
   // Entity should never be null here, but it helps to code defensively.
   om::EntityPtr entity = entity_.GetEntity();
   if (!entity) {
      finished = true;
      return;
   }

   om::MobPtr mobP = entity->GetComponent<om::Mob>();
   if (mobP) {
      csg::Point3f loc = mobP -> GetWorldLocation();
      sound_->setPosition(loc.x, loc.y, loc.z);
   }

   if (delay_ > 0 && firstPlay_ && (info.now > startTime_+ delay_) ) {
      sound_->play();
      firstPlay_ = false;
      finished = false;
   } else if (sound_->getLoop()) {
      finished = false;
   } else {
      finished = info.now > startTime_ + delay_ + sound_->getBuffer()->getDuration().asMilliseconds(); 
   }
}



