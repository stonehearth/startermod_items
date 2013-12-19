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
}

void EffectList::ExtendObject(json::Node const& obj)
{
   default_ = obj.get<std::string>("default", "");
   default_in_list_ = false;
   next_id_ = 1;
   AddRemoveDefault();
   
   for (json::Node const& entry : obj.get_node("effects")) {
      std::string path = entry.as<std::string>();
      AddEffect(path, 0);
   }
}

void EffectList::AddRemoveDefault()
{
   if(default_.Get().compare("") == 0) {
      //If there is no default, then always return
      return;
   }
   if(default_in_list_) {
      if (effects_.GetSize() >= 2) {
         //If there is a default, it's ID is always the first one, 1
         effects_.Remove(1);
         default_in_list_ = false;
      }
   } else {
      //The default is not in the list, so add it if there are no other effects
      if (effects_.GetSize() == 0) {
         auto effect = GetStore().AllocObject<Effect>();
         int effect_id = (*next_id_);
         next_id_ = (*next_id_) + 1;

         effect->Init(effect_id, default_.Get(), 0);
         effects_.Add(effect_id, effect);
         default_in_list_ = true;
      }
   }

}

EffectPtr EffectList::AddEffect(std::string const& effectName, int startTime)
{
   auto effect = GetStore().AllocObject<Effect>();
   int effect_id = (*next_id_);
   next_id_ = (*next_id_) + 1;

   effect->Init(effect_id, effectName, startTime);
   effects_.Add(effect_id, effect);
   AddRemoveDefault();
   return effect;
}

void EffectList::RemoveEffect(EffectPtr effect)
{
   effects_.Remove(effect->GetEffectId());
   AddRemoveDefault();
}

