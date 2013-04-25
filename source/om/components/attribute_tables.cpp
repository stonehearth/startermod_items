#include "pch.h"
#include "attribute_tables.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

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

AttributeTablePtr AddTable(std::string name);
AttributeTablePtr GetTable(std::string name);
