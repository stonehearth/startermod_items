#include "pch.h"
#include "effect_list.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const Effect& o)
{
   os << "[Effect " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}

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

