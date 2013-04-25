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
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/components/mob.h"
#include "om/components/render_rig.h"
#include "resources/res_manager.h"
#include "resources/animation.h"

using namespace ::radiant;
using namespace ::radiant::client;
using radiant::client::RenderInnerEffectList;

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

static void MoveSceneNode(H3DNode node, const math3d::transform& t, float scale)
{
   math3d::matrix4 m(t.orientation);
   
   m[12] = t.position.x * scale;
   m[13] = t.position.y * scale;
   m[14] = t.position.z * scale;
   
   bool result = h3dSetNodeTransMat(node, m.get_float_ptr());
   if (!result) {
      LOG(WARNING) << "failed to set transform on node.";
   }
}

RenderEffectList::RenderEffectList(RenderEntity& entity, om::EffectListPtr effectList) :
   entity_(entity),
   effectList_(effectList)
{
   ASSERT(effectList);

   auto added = std::bind(&RenderEffectList::AddEffect, this, std::placeholders::_1);
   auto removed = std::bind(&RenderEffectList::RemoveEffect, this, std::placeholders::_1);

   auto &effects = effectList->GetEffects();

   tracer_ += effects.TraceSetChanges("render rig rigs", added, removed);
   tracer_ += Renderer::GetInstance().TraceFrameStart(std::bind(&RenderEffectList::UpdateEffects, this));

   lastUpdateTime_ = Renderer::GetInstance().GetCurrentFrameTime();
   for (const om::EffectPtr effect : effects) {
      AddEffect(effect);
   }
}

RenderEffectList::~RenderEffectList()
{
}

void RenderEffectList::AddEffect(const om::EffectPtr effect)
{
   std::string name = effect->GetName();
   effects_[effect->GetObjectId()] = RenderInnerEffectList(entity_, effect);
}

void RenderEffectList::RemoveEffect(const om::EffectPtr effect)
{
   effects_.erase(effect->GetObjectId());
}

void RenderEffectList::UpdateEffects()
{
   int now = Renderer::GetInstance().GetCurrentFrameTime();
   auto i = effects_.begin();
   while (i != effects_.end()) {
      bool finished = false;
      i->second.Update(now, now - lastUpdateTime_, finished);
      if (finished) {
         i = effects_.erase(i);
      } else {
         i++;
      }
   }
   lastUpdateTime_ = now;
}

RenderInnerEffectList::RenderInnerEffectList(RenderEntity& renderEntity, om::EffectPtr effect)
{
   std::string name = effect->GetName();
   auto res = resources::ResourceManager2::GetInstance().Lookup<resources::DataResource>(name);
   if (res && res->GetType() == resources::Resource::EFFECT) {
      for (const JSONNode& node : res->GetJson()["tracks"]) {
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
         }
         if (e) {
            effects_.push_back(e);
         }
      }
   }
}

void RenderInnerEffectList::Update(int now, int dt, bool& finished)
{
   int i = 0, c = effects_.size();
   while (i < c) {
      auto effect = effects_[i];
      bool done = false;
      effect->Update(now, dt, done);
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

RenderAnimationEffect::RenderAnimationEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e)
{
   int now = effect->GetStartTime();
   animationName_ = node["animation"].as_string();
   animation_ = resources::ResourceManager2::GetInstance().Lookup<resources::Animation>(animationName_);

   if (animation_) {
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
   }
   int renderTime = Renderer::GetInstance().GetCurrentFrameTime();
   LOG(INFO) << "starting animation effect" << animationName_ << "(start_time: " << startTime_ << " current render time: " << renderTime << ")";
}


void RenderAnimationEffect::Update(int now, int dt, bool& finished)
{
   if (!animation_) {
      finished = true;
      return;
   }
   int offset = now - startTime_;
   if (duration_) {    
      if (now > startTime_ + duration_) {
         finished = true;
      }

      // if we're not looping, make sure we stop at the end frame.
      int animationDuration = (int)animation_->GetDuration();
      if (offset > animationDuration) {
         offset = animationDuration - 1;
      }
   }

   LOG(INFO) << "advancing animation " << animationName_ << " to offset " << offset << "(start_time: " << startTime_ << " now: " << now << " duration: " << duration_ << ")";
   animation_->MoveNodes(offset, [&](std::string bone, const math3d::transform &transform) {
      H3DNode node = entity_.GetSkeleton().GetSceneNode(bone);
      if (node) {
         MoveSceneNode(node, transform, entity_.GetSkeleton().GetScale());
      }
   });
}


HideBoneEffect::HideBoneEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
   boneNode_(0),
   boneNodeFlags_(0)
{
   auto i = node.find("bone");
   if (i != node.end()) {
      boneNodeFlags_ = h3dGetNodeFlags(boneNode_);
      boneNode_ = entity_.GetSkeleton().GetSceneNode(i->as_string());
   }
}

HideBoneEffect::~HideBoneEffect()
{
   if (boneNode_ ) {
      h3dSetNodeFlags(boneNode_, boneNodeFlags_, true);  
   }
}

void HideBoneEffect::Update(int now, int dt, bool& finished)
{
   if (boneNode_) {      
      h3dSetNodeFlags(boneNode_, boneNodeFlags_ | H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);
   }
   finished = true;
}

RenderAttachItemEffect::RenderAttachItemEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
   iconicOverride_(-1),
   finished_(false)
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
               mob->MoveTo(math3d::point3(0, 0, 0));
               mob->TurnToAngle(0);
            }
         }
#endif
      }
   } else {
      item = om::Stonehearth::CreateEntity(Client::GetInstance().GetAuthoringStore(), kind);
   }

   if (item) {
      startTime_ = GetStartTime(node) + now;
      bone_ = node["bone"].as_string();

      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone_);
      item_ = Renderer::GetInstance().CreateRenderObject(parent, item);
      item_->SetParent(0);

      auto i = node.find("render_info");
      if (i != node.end()) {
         auto j = i->find("iconic");
         if (j != i->end()) {
            iconicOverride_ = j->as_bool() ? 1 : 0;
         }
      }
   }
}

RenderAttachItemEffect::~RenderAttachItemEffect()
{
   if (iconicOverride_ != -1) {
      item_->ClearDisplayIconicOveride();
   }
}

void RenderAttachItemEffect::Update(int now, int dt, bool& finished)
{
   if (finished_) {
      finished = true;
      return;
   }

   if (now >= startTime_) {
      finished = true;
      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone_);
      item_->SetParent(parent);
      if (iconicOverride_ != -1) {
         item_->SetDisplayIconicOveride(iconicOverride_ != 0);
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

FloatingCombatTextEffect::FloatingCombatTextEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
   height_(0)
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
   om::EntityPtr entity = Client::GetInstance().GetStore().FetchObject<om::Entity>(e.GetEntityId());
   if (entity) {
      om::RenderRigPtr rig = entity->GetComponent<om::RenderRig>();
      if (rig) {
         std::string animationTableName = rig->GetAnimationTable();
         auto obj = resources::ResourceManager2::GetInstance().Lookup<resources::ObjectResource>(animationTableName);
         if (obj) {
            auto cs = obj->Get<resources::ObjectResource>("collision_shape");
            if (cs) {
               height_ = cs->GetFloat("height");
               height_ *= 0.1f; // xxx - take this out of the same place where we store the face that the model is 10x too big
            }
         }
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

void FloatingCombatTextEffect::Update(int now, int dt, bool& finished)
{
   if (tweener_->is_finished()) {
      if (toastNode_) {
         h3dRemoveNode(toastNode_->GetNode());
         toastNode_ = nullptr;
      }
      finished = true;
      return;
   }
   tweener_->update((double)dt);
   h3dSetNodeTransform(toastNode_->GetNode(), 0, static_cast<float>(height_), 0, 0, 0, 0, 1, 1, 1);
}
