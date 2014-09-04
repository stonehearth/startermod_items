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

class RenderEffectTrack {
public:
   RenderEffectTrack(RenderEntity& entity, int effectId, std::string const& prefix);
   virtual void Update(FrameStartInfo const& info, bool& done) = 0;

protected:
   RenderEntity&  entity_;
   std::string    log_prefix_;
};

class RenderAnimationEffectTrack : public RenderEffectTrack {
public:
   RenderAnimationEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~RenderAnimationEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   int            startTime_;
   float          duration_;
   float          speed_;
   std::string    animationName_;
   res::AnimationPtr  animation_;
};

struct RenderAttachItemEffectTrack : public RenderEffectTrack {
public:
   RenderAttachItemEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~RenderAttachItemEffectTrack();

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

struct FloatingCombatTextEffectTrack : public RenderEffectTrack {
public:
   FloatingCombatTextEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~FloatingCombatTextEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   int                           startTime_;
   int                           lastUpdated_;
   double                        height_;
   horde3d::ToastNode*           toastNode_;
   std::unique_ptr<claw::tween::single_tweener> tweener_;
};

struct HideBoneEffectTrack : public RenderEffectTrack {
public:
   HideBoneEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~HideBoneEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   std::string                   boneName_;
};

struct CubemitterEffectTrack : public RenderEffectTrack {
public:
   CubemitterEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~CubemitterEffectTrack();

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
struct LightEffectTrack : public RenderEffectTrack {
public:
   LightEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~LightEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   H3DNodeUnique      lightNode_;
};
class ::radiant::horde3d::DecalNode;

/* For creating activity effects from a given JSON file. */
struct ActivityOverlayEffectTrack : public RenderEffectTrack {
public:
   ActivityOverlayEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~ActivityOverlayEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   bool PositionOverlayNode();

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   Horde3D::HudElementNode *_hud;
   H3DRes            _matRes;
   H3DNodeUnique     overlayNode_;
   int               _overlayWidth;
   int               _overlayHeight;
   int               _yOffset;
   std::string       _material;
   bool              _positioned;
};


/* For creating unit overlay effects from a given JSON file. */
struct UnitStatusEffectTrack : public RenderEffectTrack {
public:
   UnitStatusEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~UnitStatusEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   void parseTransforms(const JSONNode& node, float* x, float* y, float* z);
   H3DNodeUnique      statusNode_;
   H3DRes             _matRes;
};

/* For playing short sound effects*/

struct PlaySoundEffectTrack : public RenderEffectTrack {
public:
   PlaySoundEffectTrack(RenderEntity& e, om::EffectPtr effect, const JSONNode& node);
   ~PlaySoundEffectTrack();

   void Update(FrameStartInfo const& info, bool& done) override;

private:
   std::shared_ptr<sf::Sound>       sound_;
   int             effectStartTime_;
   int             startTime_;   //Time when the sound starts to play
   int             endTime_;     //Time when the sound should end (or 0 if natural duration)
   bool            firstPlay_;   //Whether this is the first time we're playing the sound
   int             maxDistance_; //distance under which sound will be heard at maximum volume. 1 is default
   double          volume_;      //Volume specified by json for sound

   void  AssignFromJSON_(const JSONNode& node);
   float CalculateAttenuation(int maxDistance, int minDistance);

};

struct RenderEffect {
   RenderEffect() {}
   RenderEffect(RenderEntity& e, om::EffectPtr effect);
   ~RenderEffect();

   void Update(FrameStartInfo const& info, bool& finished);
private:
   int                  _effectId;
   std::string          log_prefix_;
   std::vector<std::shared_ptr<RenderEffectTrack>>   tracks_;
   std::vector<std::shared_ptr<RenderEffectTrack>>   finished_;
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
   typedef std::unordered_map<dm::ObjectId, RenderEffect> EffectMap;

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
