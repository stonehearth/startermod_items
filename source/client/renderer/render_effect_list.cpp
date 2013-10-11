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
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "radiant_json.h"
#include "lua/script_host.h"
#include <SFML/Audio.hpp>

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

static void MoveSceneNode(H3DNode node, const csg::Transform& t, float scale)
{
   csg::Matrix4 m(t.orientation);
   
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
   try {
      std::string name = effect->GetName();
      JSONNode const& data = res::ResourceManager2::GetInstance().LookupJson(name);
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
         } else if (type == "music_effect") {
            //Use this class if you just want simple, single-track background music
            //e = std::make_shared<PlayMusicEffect>(renderEntity, effect, node); 
         
            //Use this if you want the current bg music to fade out before new bg music starts to play
            //TODO: remove this class when we have lua event interrupting
            std::shared_ptr<SingMusicEffect> m = SingMusicEffect::GetMusicInstance(renderEntity);
            m ->PlayMusic(effect, node); 
            e = m;
         } else if (type == "sound_effect") {
            if (PlaySoundEffect::ShouldCreateSound()) {
               e = std::make_shared<PlaySoundEffect>(renderEntity, effect, node); 
            }
         } else if (type == "cubemitter") {
            e = std::make_shared<CubemitterEffect>(renderEntity, effect, node);
         } else if (type == "light") {
            e = std::make_shared<LightEffect>(renderEntity, effect, node);
         }
         if (e) {
            effects_.push_back(e);
         }
      }
   } catch (std::exception& e) {
      LOG(WARNING) << "failed to create effect: " << e.what();
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

///////////////////////////////////////////////////////////////////////////////
// RenderAnimationEffect
///////////////////////////////////////////////////////////////////////////////

RenderAnimationEffect::RenderAnimationEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e)
{
   int now = effect->GetStartTime();
   animationName_ = node["animation"].as_string();
   
   // fuck
   std::string animationTable = e.GetEntity()->GetComponent<om::RenderInfo>()->GetAnimationTable();
   json::ConstJsonObject json = res::ResourceManager2::GetInstance().LookupJson(animationTable);
   std::string animationRoot = json.get<std::string>("animation_root", "");

   animationName_ = animationRoot + "/" + animationName_;
   animation_ = res::ResourceManager2::GetInstance().LookupAnimation(animationName_);

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

   animation_->MoveNodes(offset, [&](const std::string& bone, const csg::Transform &transform) {
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

///////////////////////////////////////////////////////////////////////////////
// CubemitterEffect
///////////////////////////////////////////////////////////////////////////////

CubemitterEffect::CubemitterEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
   cubemitterNode_(0)
{
   auto cubemitterFileName = node["cubemitter"].as_string();

   H3DRes matRes = h3dAddResource(H3DResTypes::Material, "materials/cubemitter.material.xml", 0);
   H3DRes cubeRes = h3dAddResource(RT_CubemitterResource, cubemitterFileName.c_str(), 0);

   H3DNode c = h3dRadiantAddCubemitterNode(e.GetNode(), "cu", cubeRes, matRes);
   cubemitterNode_ = H3DCubemitterNodeUnique(c);
   float x, y, z, rx, ry, rz;

   parseTransforms(node["transforms"], &x, &y, &z, &rx, &ry, &rz);
   h3dSetNodeTransform(cubemitterNode_.get(), x, y, z, rx, ry, rz, 1, 1, 1);
}

void CubemitterEffect::parseTransforms(const JSONNode& node, float *x, float *y, float *z, float *rx, float *ry, float *rz)
{
   radiant::json::ConstJsonObject o(node);

   *x = o.get("x", 0.0f);
   *y = o.get("y", 0.0f);
   *z = o.get("z", 0.0f);
   *rx = o.get("rx", 0.0f);
   *ry = o.get("ry", 0.0f);
   *rz = o.get("rz", 0.0f);
}

CubemitterEffect::~CubemitterEffect()
{
}

void CubemitterEffect::Update(int now, int dt, bool& finished)
{
   finished = false;
}

///////////////////////////////////////////////////////////////////////////////
// LightEffect
///////////////////////////////////////////////////////////////////////////////

LightEffect::LightEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
   lightNode_(0)
{
   auto animatedLightFileName = node["light"].as_string();
   // TODO: for just a moment, hardcode some light values.
   H3DRes matRes = h3dAddResource(H3DResTypes::Material, "materials/deferred_light.material.xml", 0);
   H3DRes lightRes = h3dAddResource(RT_AnimatedLightResource, animatedLightFileName.c_str(), 0);
   H3DNode l = h3dRadiantAddAnimatedLightNode(e.GetNode(), "ln", lightRes, matRes);

   float x, y, z;

   parseTransforms(node["transforms"], &x, &y, &z);
   h3dSetNodeTransform(l, x, y, z, 0, 0, 0, 1, 1, 1);

   lightNode_ = H3DNodeUnique(l);
}

void LightEffect::parseTransforms(const JSONNode& node, float* x, float* y, float* z)
{
   radiant::json::ConstJsonObject o(node);

   *x = o.get("x", 0.0f);
   *y = o.get("y", 0.0f);
   *z = o.get("z", 0.0f);
}


LightEffect::~LightEffect()
{
}

void LightEffect::Update(int now, int dt, bool& finished)
{
   finished = false;
}

///////////////////////////////////////////////////////////////////////////////
// RenderAttachItemEffect
///////////////////////////////////////////////////////////////////////////////

RenderAttachItemEffect::RenderAttachItemEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
   entity_(e),
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
         LOG(WARNING) << "!!!!!!! ERROR IN RenderAttachItemEffect: " << e.what();
      }
   }

   if (item) {
      startTime_ = GetStartTime(node) + now;
      bone_ = node["bone"].as_string();

      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone_);
      render_item_ = Renderer::GetInstance().CreateRenderObject(parent, item);
      render_item_->SetParent(0);

      auto i = node.find("render_info");
      if (i != node.end()) {
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
      render_item_->SetModelVariantOverride(false, "");
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
      render_item_->SetParent(parent);
      if (use_model_variant_override_) {
         render_item_->SetModelVariantOverride(true, model_variant_override_);
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
   om::EntityPtr entity = Client::GetInstance().GetStore().FetchObject<om::Entity>(e.GetObjectId());
   if (entity) {
      om::RenderInfoPtr render_info = entity->GetComponent<om::RenderInfo>();
      if (render_info) {
         std::string animationTableName = render_info->GetAnimationTable();

         json::ConstJsonObject json = res::ResourceManager2::GetInstance().LookupJson(animationTableName);
         json::ConstJsonObject cs = json.get<JSONNode>("collision_shape");
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

/* Sound Effect Classes 
 * See http://wiki.rad-ent.com/doku.php?id=adding_music_and_sound_effects&#adding_a_sound_effect 
 * for a full description of how to create a sound effect in json that is then interpreted here.
 */

/* PlayMusicEffect
 * Constructor
 * Simple class to play background music, currently not in use
 * TODO: when we have the ability to interrupt effects, return to using this
 * and add code to quiet the music as it fades.
**/
#define PLAY_MUSIC_EFFECT_DEFAULT_LOOP true;

PlayMusicEffect::PlayMusicEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node) :
	entity_(e)
{
   startTime_ = effect->GetStartTime();
   std::string trackName = node["track"].as_string();
   try {
      trackName = res::ResourceManager2::GetInstance().GetResourceFileName(trackName, "");
   } catch (res::Exception& e) {
      LOG(WARNING) << "could not load music effect: " << e.what();
      return;
   }
   
   loop_ = PLAY_MUSIC_EFFECT_DEFAULT_LOOP;

   auto i = node.find("loop");
   if (i != node.end()) {
      loop_ = (bool)i->as_bool();
   }

   if (music_.openFromFile(trackName)) {
      music_.setLoop(loop_);
      music_.play();
   } else { 
      LOG(INFO) << "Can't find Music! " << trackName;
   }
}

PlayMusicEffect::~PlayMusicEffect()
{
    //Nothing actually needs to be destroyed here
}

/* PlayMusicEffect::Update
 * On update, check if we are finished.
 * If we're looping, we're never finished. If we aren't looping,
 * then finished is true if we're past the start time and music duration.
 * TODO: implement delay for background music?
**/
void PlayMusicEffect::Update(int now, int dt, bool& finished)
{
   if (music_.getLoop()) {
      finished = false;
   } else {
      finished = now > startTime_ + music_.getDuration().asMilliseconds(); 
   }
}

/* "Singleton" version of PlayMusicEffect*
 * Uses GetMusicInstance to call the constructor, so that we can be sure to
 * quiet the old music before playing the new music passed in. 
 * TODO: Replace with simple verson once we have a mechanism to quiet music.
**/
#define SING_MUSIC_EFFECT_DEF_VOL 40; //I like my music quieter than my sound effects
#define SING_MUSIC_EFFECT_FADE 2000;  //MS to have old music fade
#define SING_MUSIC_EFFECT_DEFAULT_LOOP true;

std::shared_ptr<SingMusicEffect> SingMusicEffect::music_instance_ = NULL;

/* SingMusicEffect::GetMusicInstance
 * Use this static function to get the music instance.
 * Create a new one if necessary
 * Returns a shared ptr to a SingMusicInstance.
**/
std::shared_ptr<SingMusicEffect> SingMusicEffect::GetMusicInstance(RenderEntity& e)
{
   if (!music_instance_) {   
      //TODO: If we want music to be tied to a particular activity, and fade as we get further
      //we may need to create a new instance each time since the entity can only be assigned in the constructor
      music_instance_ = std::make_shared<SingMusicEffect>(e);
    }
    return music_instance_;
}

/* SingMusicEffect::PlayMusic
 * Call whenever we have a new song to play
**/
void SingMusicEffect::PlayMusic(om::EffectPtr effect, const JSONNode& node)
{   
   startTime_ = effect->GetStartTime();

   std::string trackName = node["track"].as_string();
   try {
      trackName = res::ResourceManager2::GetInstance().GetResourceFileName(trackName, "");
   } catch (res::Exception& e) {
      LOG(WARNING) << "could not load music: " << e.what();
      return;
   }
   
   loop_ = SING_MUSIC_EFFECT_DEFAULT_LOOP;

   auto i = node.find("loop");
   if (i != node.end()) {
      loop_ = (bool)i->as_bool();
   }

   //If there is already music playing, note next track for update
   if (music_.getStatus() == sf::Music::Playing) {
     nextTrack_ = trackName;
   } else {
      //If there is no music, immediately start bg music
      if (music_.openFromFile(trackName)) {
         music_.setLoop(loop_);
         music_.setVolume((float)volume_);
	      music_.play();
      } else { 
         LOG(INFO) << "Can't find Music! " << trackName;
      }
   }
}

/* SingMusicEffect
 * Constructor (Public, but do not call!)
 * Unfortunately, can't make this private because the shared ptr expects it
**/
SingMusicEffect::SingMusicEffect(RenderEntity& e) : 
   entity_(e)
{
   nextTrack_ = "";
   volume_ = SING_MUSIC_EFFECT_DEF_VOL;
   tweener_ = NULL;
}

SingMusicEffect::~SingMusicEffect()
{
}

/* SingMusicEffect::Update
 * On update, keep playing the current BG music, unless there is a new music track
 * If so, fade out the old one and eventually introduce the new one. 
**/
void SingMusicEffect::Update(int now, int dt, bool& finished)
{
   if (music_.getLoop()) {
      finished = false;
   } else {
      finished = now > startTime_ + music_.getDuration().asMilliseconds(); 
   }

   //If there is another track to play after this one
   if (nextTrack_.length() > 0) {
      if (!tweener_.get()) {
         //if we have another track to do and the tweener is null, then create a new tweener
         int fade = SING_MUSIC_EFFECT_FADE;
         tweener_.reset(new claw::tween::single_tweener(volume_, 0, fade, get_easing_function("sine_out")));
      } else {
         //There is a next track and a tweener, we must be in the process of quieting the prev music
         if (tweener_->is_finished()) {
            //If the tweener is finished, play the next track
            tweener_ = NULL;
            music_.stop();
            if (music_.openFromFile(nextTrack_)) {
            //TODO: crossfade
            volume_ = SING_MUSIC_EFFECT_DEF_VOL;
            music_.setVolume((float)volume_);
            music_.setLoop(loop_);
            music_.play();
            } else { 
               LOG(INFO) << "Can't find Music! " << nextTrack_;
            }
            nextTrack_ = "";
         } else {
            //If the tweener is not finished, soften the volume
            tweener_->update((double)dt);
            music_.setVolume((float)volume_);
         }
      }
   }
}


/* PlaySoundEffect
 * Class to play a short sound. Loads sound into memory.
 * Can only play 250 sounds at once; limitation of sfml
 * Be sure to call ShouldCreateSound() before calling the constructor.
 * TODO: Theoretically, we could keep a hash of all the soundbuffers
 * so we could just have 1 buffer per type of sound; revisit if
 * optimization is required.
 * TODO: if we run out of sounds too quickly, only create sounds if the
 * camera is in range of them. (Make sound a ptr and create/dispose as necessary)
**/

//Static variable to keep track of the total number of sounds
int PlaySoundEffect::numSounds_ = 0;

/* PlaySoundEffect::ShouldCreateSound
 * Static function to check if we already have more than 250 sounds. If so
 * returns false. Otherwise, returns true. Call before calling the constructor.
*/
bool PlaySoundEffect::ShouldCreateSound() {
   return (numSounds_ <= 250);
}

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
	entity_(e)
{
   startTime_ = effect->GetStartTime();
   firstPlay_ = true;
   numSounds_++;

   std::string trackName;
   try {
      trackName = res::ResourceManager2::GetInstance().GetResourceFileName(
         node["track"].as_string(), "");
   } catch (std::exception& e) {
      LOG(WARNING) << "could not load sound effect: " << e.what();
      return;
   }

   if (soundBuffer_.loadFromFile(trackName)) {
      sound_.setBuffer(soundBuffer_);
      AssignFromJSON_(node);
      if (delay_ == 0) {
         //TODO: if camera is farther than maxDistance, create sound and play
	      sound_.play();
      }
   } else { 
      LOG(INFO) << "Can't find Sound Effect! " << trackName;
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

   attenuation = CalculateAttenuation_(maxDistance, minDistance);

   //Set member variables
   delay_ = delay;
   maxDistance_ = maxDistance; 

   //Set sound parameters
   sound_.setLoop(loop);
   sound_.setAttenuation(attenuation);
   sound_.setMinDistance((float)minDistance);
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
float PlaySoundEffect::CalculateAttenuation_(int maxDistance, int minDistance) {
   float interm1 = maxDistance / PLAY_SOUND_EFFECT_MIN_VOLUME;
   float interm2 = interm1 - minDistance;
   float interm3 = (float)(maxDistance - minDistance);
   return (interm2/interm3);
}

PlaySoundEffect::~PlaySoundEffect()
{
    numSounds_--;
    //TODO: if we're showing sounds based on camera location, dispose of the sound object here
}

/* PlaySoundEffect::Update
 * Update location of sound.
 * If we were delaying the sound start and now is the time to actually start the sound, start it. 
 * If the sound was in a loop, we are not yet finished.
 * If the sound is finished, set finished to true
 * TODO: on update, check if camera is between min and max distances. If so, create/destroy sound as necessary
**/
void PlaySoundEffect::Update(int now, int dt, bool& finished)
{
   om::MobPtr mobP = entity_.GetEntity()->GetComponent<om::Mob>();
   if (mobP) {
      csg::Point3f loc = mobP -> GetWorldLocation();
      sound_.setPosition(loc.x, loc.y, loc.z);
   }

   if (delay_ > 0 && firstPlay_ && (now > startTime_+ delay_) ) {
      sound_.play();
      firstPlay_ = false;
      finished = false;
   } else if (sound_.getLoop()) {
      finished = false;
   } else {
      finished = now > startTime_ + delay_ + soundBuffer_.getDuration().asMilliseconds(); 
   }

}



