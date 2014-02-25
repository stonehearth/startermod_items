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

void EffectList::ExtendObject(json::Node const& obj)
{
   default_ = obj.get<std::string>("default", "");
   default_in_list_ = 0;
   next_id_ = 1;
   AddRemoveDefault();
   
   for (json::Node const& entry : obj.get_node("effects")) {
      std::string path = entry.as<std::string>();
      AddEffect(path, 0);
   }
}

void EffectList::AddRemoveDefault()
{
   if ((*default_).empty()) {
      //If there is no default, then always return
      return;
   }
   if (default_in_list_ > 0) {
      if (effects_.GetSize() >= 2) {
         effects_.Remove(default_in_list_);
         default_in_list_ = 0;
      }
   } else {
      //The default is not in the list, so add it if there are no other effects
      if (effects_.GetSize() == 0) {
         auto effect = CreateEffect(*default_, 0);
         default_in_list_ = effect->GetEffectId();
      }
   }

}

EffectPtr EffectList::AddEffect(std::string const& effectName, int startTime)
{
   auto effect = CreateEffect(effectName, startTime);
   AddRemoveDefault();
   return effect;
}

EffectPtr EffectList::CreateEffect(std::string const& effectName, int startTime)
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
   AddRemoveDefault();
}

