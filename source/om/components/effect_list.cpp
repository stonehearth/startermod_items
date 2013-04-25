#include "pch.h"
#include "effect_list.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Effect::AddParam(std::string name, luabind::object o)
{
   params_[name].FromLuaObject(o);
}

EffectPtr EffectList::AddEffect(std::string effectName, int startTime)
{
   auto effect = GetStore().AllocObject<Effect>();
   effect->Init(effectName, startTime);
   effects_.Insert(effect);
   return effect;
}

void EffectList::RemoveEffect(EffectPtr effect)
{
   for (EffectPtr e : effects_) {
      if (e == effect) {
         effects_.Remove(e);
         break;
      }
   }
}

