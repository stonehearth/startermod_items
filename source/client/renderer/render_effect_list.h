#ifndef _RADIANT_CLIENT_RENDER_EFFECT_H
#define _RADIANT_CLIENT_RENDER_EFFECT_H

#include <map>
#include <claw/tween/single_tweener.hpp>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "types.h"
#include "skeleton.h"
#include "render_component.h"
#include "resources/animation.h"
#include "om/components/effect_list.h"
#include <SFML\Audio.hpp>

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class RenderEntity;

class RenderEffect {
public:
   virtual void Update(int now, int dt, bool& done) = 0;
};

class RenderAnimationEffect : public RenderEffect {
public:
   RenderAnimationEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);

   void Update(int now, int dt, bool& done) override;

private:
   RenderEntity&  entity_;
   int            startTime_;
   float          duration_;
   std::string    animationName_;
   resources::AnimationPtr  animation_;
};

struct RenderAttachItemEffect : public RenderEffect {
public:
   RenderAttachItemEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~RenderAttachItemEffect();

   void Update(int now, int dt, bool& done) override;

private:
   RenderEntity&                 entity_;
   std::shared_ptr<RenderEntity> item_;
   bool                          finished_;
   int                           startTime_;
   int                           iconicOverride_;
   std::string                   bone_;
};

struct FloatingCombatTextEffect : public RenderEffect {
public:
   FloatingCombatTextEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~FloatingCombatTextEffect();

   void Update(int now, int dt, bool& done) override;

private:
   RenderEntity&                 entity_;
   int                           startTime_;
   double                        height_;
   horde3d::ToastNode*           toastNode_;
   std::unique_ptr<claw::tween::single_tweener> tweener_;
};

struct HideBoneEffect : public RenderEffect {
public:
   HideBoneEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~HideBoneEffect();

   void Update(int now, int dt, bool& done) override;

private:
   RenderEntity&                 entity_;
   H3DNode                       boneNode_;
   int                           boneNodeFlags_;
};

/* For playing background music*/
struct PlayMusicEffect : public RenderEffect {
public:
	PlayMusicEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
	~PlayMusicEffect();

	void Update(int now, int dt, bool& done) override;

private:
	RenderEntity&	entity_;
   sf::Music      music_;
   bool           loop_;
};

struct RenderInnerEffectList {
   RenderInnerEffectList() {}
   RenderInnerEffectList(RenderEntity& e, om::EffectPtr effect);

   void Update(int now, int dt, bool& finished);
private:
   std::vector<std::shared_ptr<RenderEffect>>   effects_;
   std::vector<std::shared_ptr<RenderEffect>>   finished_;
};

class RenderEffectList : public RenderComponent {
public:
   RenderEffectList(RenderEntity& entity, om::EffectListPtr effectList);
   ~RenderEffectList();

private:
   void AddEffect(const om::EffectPtr effect);
   void RemoveEffect(const om::EffectPtr effect);
   void UpdateEffects();

private:
   typedef std::unordered_map<dm::ObjectId, RenderInnerEffectList> EffectMap;

private:
   RenderEntity&        entity_;
   om::EffectListRef    effectList_;
   dm::GuardSet         tracer_;
   EffectMap            effects_;
   int                  lastUpdateTime_;
   int                  dt;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_EFFECT_H
