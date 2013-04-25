#include <luabind/luabind.hpp>
#include "pch.h"
#include "aura_list.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void AuraList::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("auras", auras_);
}

AuraRef AuraList::GetAura(std::string name, om::EntityRef s)
{
   om::EntityPtr source = s.lock();
   for (om::AuraPtr aura : auras_) {
      if (name == aura->GetName()) {
         EntityPtr entity = aura->GetSource().lock();
         if (source == entity) {
            return aura;
         }
      }
   }
   return AuraRef();
}

AuraRef AuraList::CreateAura(std::string name, om::EntityRef source)
{
   AuraPtr aura = GetStore().AllocObject<Aura>();

   aura->Construct(name, source);
   auras_.Insert(aura);
   return aura;
}

void AuraList::RemoveAura(AuraPtr aura)
{
   auras_.Remove(aura);
}
