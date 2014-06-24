#ifndef _RADIANT_CLIENT_RENDER_EFFECT_H
#define _RADIANT_CLIENT_RENDER_EFFECT_H

#include <map>
#include <claw/tween/single_tweener.hpp>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "skeleton.h"
#include "render_component.h"
#include "resources/animation.h"
#include "om/components/effect_list.ridl.h"
#include "lib/audio/input_stream.h"
#include <SFML/Audio.hpp>

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class RenderEntity;

class RenderEffect {
public:
   RenderEffect(RenderEntity& entity, std::string const& prefix);
   virtual void Update(FrameStartInfo const& info, bool& done) = 0;

protected:
   RenderEntity&  entity_;
   std::string    log_prefix_;
};

class RenderAnimationEffect : public RenderEffect {
public:
   RenderAnimationEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~RenderAnimationEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   int            startTime_;
   float          duration_;
   float          speed_;
   std::string    animationName_;
   res::AnimationPtr  animation_;
};

struct RenderAttachItemEffect : public RenderEffect {
public:
   RenderAttachItemEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~RenderAttachItemEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   std::shared_ptr<om::Entity>   authored_entity_;
   std::shared_ptr<RenderEntity> render_item_;
   bool                          finished_;
   int                           startTime_;
   std::string                   model_variant_override_;
   bool                          use_model_variant_override_;
   std::string                   bone_;
};

struct FloatingCombatTextEffect : public RenderEffect {
public:
   FloatingCombatTextEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~FloatingCombatTextEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   int                           startTime_;
   int                           lastUpdated_;
   double                        height_;
   horde3d::ToastNode*           toastNode_;
   std::unique_ptr<claw::tween::single_tweener> tweener_;
};

struct HideBoneEffect : public RenderEffect {
public:
   HideBoneEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~HideBoneEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   H3DNode                       boneNode_;
   int                           boneNodeFlags_;
};

/* For creating cubemitters from a given JSON file. */
struct CubemitterEffect : public RenderEffect {
public:
   CubemitterEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~CubemitterEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   int                           startTime_;
   int                           endTime_;
   H3DCubemitterNodeUnique       cubemitterNode_;
   std::string                   filename_;
   csg::Point3f                  pos_;
   csg::Point3f                  rot_;
   H3DNode                       parent_;
};

/* For creating lights from a given JSON file. */
struct LightEffect : public RenderEffect {
public:
   LightEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~LightEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   H3DNodeUnique      lightNode_;
};
class ::radiant::horde3d::DecalNode;

/* For creating activity effects from a given JSON file. */
struct ActivityOverlayEffect : public RenderEffect {
public:
   ActivityOverlayEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~ActivityOverlayEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   bool PositionOverlayNode();

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   Horde3D::HudElementNode *_hud;
   H3DNodeUnique     overlayNode_;
   int               _overlayWidth;
   int               _overlayHeight;
   int               _yOffset;
   std::string       _material;
   bool              _positioned;
};


/* For creating unit overlay effects from a given JSON file. */
struct UnitStatusEffect : public RenderEffect {
public:
   UnitStatusEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~UnitStatusEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   H3DNodeUnique      statusNode_;
};

/* For playing short sound effects*/

struct PlaySoundEffect : public RenderEffect {
public:
   PlaySoundEffect(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~PlaySoundEffect();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   std::shared_ptr<sf::Sound>       sound_;
   int             startTime_;   //Time when the sound starts to play
   bool            firstPlay_;   //Whether this is the first time we're playing the sound
   int             delay_;       //How long to wait before starting the sound
   int             maxDistance_; //distance under which sound will be heard at maximum volume. 1 is default

   void  AssignFromJSON_(const JSONNode& node);
   float CalculateAttenuation(int maxDistance, int minDistance);

};

struct RenderInnerEffectList {
   RenderInnerEffectList() {}
   RenderInnerEffectList(RenderEntity& e, om::EffectPtr effect);
   ~RenderInnerEffectList();

   void Update(FrameStartInfo const& info, bool& finished);
private:
   std::string          log_prefix_;
   std::vector<std::shared_ptr<RenderEffect>>   effects_;
   std::vector<std::shared_ptr<RenderEffect>>   finished_;
};

class RenderEffectList : public RenderComponent {
public:
   RenderEffectList(RenderEntity& entity, om::EffectListPtr effectList);
   ~RenderEffectList();

private:
   void AddEffect(int effect_id, const om::EffectPtr effect);
   void RemoveEffect(int effect_id);
   void UpdateEffects(FrameStartInfo const& info);

private:
   typedef std::unordered_map<dm::ObjectId, RenderInnerEffectList> EffectMap;

private:
   RenderEntity&        entity_;
   std::string          log_prefix_;
   om::EffectListRef    effectList_;
   core::Guard          renderer_guard_;
   dm::TracePtr         effects_list_trace_;
   EffectMap            effects_;
   int                  dt;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_EFFECT_H
