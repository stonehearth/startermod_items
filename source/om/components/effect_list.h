#ifndef _RADIANT_OM_EFFECT_TRACKS_H
#define _RADIANT_OM_EFFECT_TRACKS_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "om/selection.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Effect : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(Effect, effect);

   std::string GetName() const { return *effectName_; }
   int GetStartTime() const { return *startTime_; }

   void Init(std::string name, int start) {
      effectName_ = name;
      startTime_ = start;
   }

   
   void AddParam(std::string param, luabind::object o);
   const Selection& GetParam(std::string param) const {
      static const Selection null;
      auto i = params_.find(param);
      return i != params_.end() ? i->second : null;
   }

private:
   void InitializeRecordFields() override {
      AddRecordField("startTime", startTime_);
      AddRecordField("effectName", effectName_);
      AddRecordField("params", params_);
   }

private:
   dm::Boxed<int>             startTime_;
   dm::Boxed<std::string>     effectName_;
   dm::Map<std::string, Selection> params_;
};

typedef std::shared_ptr<Effect> EffectPtr;

class EffectList : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(EffectList, effect_list);

   const dm::Set<EffectPtr>& GetEffects() const { return effects_; }

   EffectPtr AddEffect(std::string effectName, int startTime);
   void RemoveEffect(EffectPtr effect);

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("effects", effects_);
   }


public:
   dm::Set<std::shared_ptr<Effect>>    effects_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_EFFECT_TRACKS_H
