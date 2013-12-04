#include "pch.h"
#include "effect.ridl.h"
#include "effect_list.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, EffectList const& o)
{
   return (os << "[EffectList]");
}

void EffectList::ConstructObject()
{
   next_id_ = 1;
}

void EffectList::ExtendObject(json::Node const& obj)
{
   for (json::Node const& entry : obj.getn("effects")) {
      std::string path = entry.as<std::string>();
      AddEffect(path, 0);
   }
}

EffectPtr EffectList::AddEffect(std::string const& effectName, int startTime)
{
   auto effect = GetStore().AllocObject<Effect>();
   int effect_id = (*next_id_);
   next_id_ = (*next_id_) + 1;

   effect->Init(effect_id, effectName, startTime);
   effects_.Add(effect_id, effect);
   return effect;
}

void EffectList::RemoveEffect(EffectPtr effect)
{
   effects_.Remove(effect->GetEffectId());
}

