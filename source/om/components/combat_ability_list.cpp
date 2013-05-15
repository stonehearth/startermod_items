#include "pch.h"
#include "combat_ability_list.h"
#include "om/entity.h"
#include "resources/data_resource.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

template <std::string (CombatAbility::*ptr_to_mem)() const>
static luabind::object StrFn(struct lua_State* L, const CombatAbility& ca)
{
   std::string result = (ca.*ptr_to_mem)();
   if (result.empty()) {
      return luabind::object();
   }
   return luabind::object(L, result);
}

luabind::scope CombatAbility::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;

   return
      class_<CombatAbility, CombatAbilityPtr>(name)
         .def(tostring(self))
         .def("get_json",                 &om::CombatAbility::GetJsonString)
         .def("get_priority",             &om::CombatAbility::GetPriority)
         .def("get_cooldown",             &om::CombatAbility::GetCooldown)
         .def("has_tag",                  &om::CombatAbility::HasTag)
         .def("get_script_name",          &StrFn<&om::CombatAbility::GetScriptName>)
         .def("get_script_info_string",   &StrFn<&om::CombatAbility::GetScriptInfoStr>)
         .def("get_resource_name",        &om::CombatAbility::GetResourceName)
         .def("set_resource_name",        &om::CombatAbility::SetResourceName)
         .def("get_cooldown_expire_time", &om::CombatAbility::GetCooldownExpireTime)
         .def("set_cooldown_expire_time", &om::CombatAbility::SetCooldownExpireTime)
      ;
}

std::string CombatAbility::ToString() const
{
   std::ostringstream os;
   os << "(CombatAbility id:" << GetObjectId() << " name:" << *resourceName_ << ")";
   return os.str();
}

std::string CombatAbility::GetScriptName() const
{
   return GetJson()["action_script_name"].as_string();
}

std::string CombatAbility::GetScriptInfoStr() const
{
   return GetJson()["action_script_info"].write();
}

int CombatAbility::GetCooldown() const
{
   return GetJson()["cooldown"].as_int();
}

bool CombatAbility::HasTag(std::string tag) const
{
   for (const auto& entry : GetJson()["tags"]) {
      if (entry.as_string() == tag) {
         return true;
      }
   }
   return false;
}

int CombatAbility::GetPriority() const
{
   return GetJson()["priority"].as_int();
}

const JSONNode& CombatAbility::GetJson() const
{
   auto obj = resources::ResourceManager2::GetInstance().Lookup<resources::DataResource>(*resourceName_);
   return obj->GetJson();
}

luabind::scope CombatAbilityList::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<CombatAbilityList, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("add_combat_ability",               &om::CombatAbilityList::AddCombatAbility)
         .def("get_combat_ability",               &om::CombatAbilityList::GetCombatAbility)
         .def("remove_combat_ability",            &om::CombatAbilityList::RemoveCombatAbility)
         .def("get_combat_abilities",             &om::CombatAbilityList::GetCombatAbilities)
         .def("set_target",                       &om::CombatAbilityList::SetTarget)
         .def("get_target",                       &om::CombatAbilityList::GetTarget)
         .def("clear_target",                     &om::CombatAbilityList::ClearTarget)
      ;
}

std::string CombatAbilityList::ToString() const
{
   std::ostringstream os;
   os << "(CombatAbilityList id:" << GetObjectId() << " size:" << abilities_.GetSize() << ")";
   return os.str();
}

CombatAbilityPtr CombatAbilityList::GetCombatAbility(std::string name)
{
   return abilities_.Lookup(name, nullptr);
}

CombatAbilityPtr CombatAbilityList::AddCombatAbility(std::string name)
{   
   if (!abilities_[name]) {
      auto combat_ability = GetStore().AllocObject<CombatAbility>();
      combat_ability->SetResourceName(name);
      abilities_[name] = combat_ability;
   }
   return abilities_[name];
}

void CombatAbilityList::RemoveCombatAbility(std::string name)
{
   abilities_.Remove(name);
}

om::EntityRef CombatAbilityList::GetTarget() const
{
   return *target_;
}

void CombatAbilityList::SetTarget(om::EntityRef target)
{
   target_ = target;
}

void CombatAbilityList::ClearTarget()
{
   target_ = EntityRef();
}

