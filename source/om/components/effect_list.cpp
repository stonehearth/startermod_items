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
   Component::ConstructObject();
   default_effect_id_ = 0;
   next_id_ = 1;
}

void EffectList::LoadFromJson(json::Node const& obj)
{
   default_ = obj.get<std::string>("default", "");
   AddRemoveDefault();
   
   for (json::Node const& entry : obj.get_node("effects")) {
      std::string path = entry.as<std::string>();
      AddEffect(path, 0);
   }
}

void EffectList::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("default", *default_);
   for (auto const& entry : EachEffect()) {
      json::Node effect;
      entry.second->SerializeToJson(effect);
      node.set(stdutil::ToString(entry.first), effect);
   }
}

void EffectList::AddRemoveDefault()
{
   if ((*default_).empty()) {
      //If there is no default, then always return
      return;
   }
   if (default_effect_id_ > 0) {
      if (effects_.GetSize() >= 2) {
         effects_.Remove(default_effect_id_);
         default_effect_id_ = 0;
      }
   } else {
      //The default is not in the list, so add it if there are no other effects
      if (effects_.GetSize() == 0) {
         auto effect = CreateEffect(*default_, 0);
         default_effect_id_ = effect->GetEffectId();
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

void EffectList::OnLoadObject(dm::SerializationType r)
{
   // Remove all effects from an Entity after loading.  Whoever put the effect on there
   // to begin with is responsible for putting it back at load time.
   if (r == dm::PERSISTANCE) {
      effects_.Clear();
      AddRemoveDefault();
   }
}