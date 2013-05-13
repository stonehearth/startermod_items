#ifndef _RADIANT_OM_COMBAT_ABILITIES_H
#define _RADIANT_OM_COMBAT_ABILITIES_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"
#include "libjson.h"

BEGIN_RADIANT_OM_NAMESPACE

class CombatAbilityList;

class CombatAbility : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(CombatAbility);
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   CombatAbility() : dm::Record() { }
   std::string ToString() const;

   std::string GetResourceName() const { return *resourceName_; }
   void SetResourceName(std::string name) { resourceName_ = name; }

   int GetCooldownExpireTime() const { return *cooldownExpireTime_; }
   void SetCooldownExpireTime(int time) { cooldownExpireTime_ = time; }

   std::string GetScriptName() const;
   std::string GetScriptInfoStr() const;
   bool HasTag(std::string tag) const;
   int GetCooldown() const;
   int GetPriority() const;
   std::string GetJsonString() const { return GetJson().write(); }

private:
   void InitializeRecordFields() override {
      AddRecordField("resource_name",        resourceName_);
      AddRecordField("cooldown_expire_time", cooldownExpireTime_);
      cooldownExpireTime_ = 0;
   }
   const JSONNode& GetJson() const;

private:
   dm::Boxed<int>             cooldownExpireTime_;
   dm::Boxed<std::string>     resourceName_;
   luabind::object            scriptObj_;
};

typedef std::shared_ptr<CombatAbility> CombatAbilityPtr;

class CombatAbilityList : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(CombatAbilityList);
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   CombatAbilityList() { }
   std::string ToString() const;

   const dm::Map<std::string, CombatAbilityPtr>& GetCombatAbilities() const { return abilities_; }

   CombatAbilityPtr GetCombatAbility(std::string name);
   CombatAbilityPtr AddCombatAbility(std::string name);
   void RemoveCombatAbility(std::string name);

   om::EntityRef GetTarget() const;
   void SetTarget(om::EntityRef target);
   void ClearTarget();

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("abilities",     abilities_);
      AddRecordField("target",        target_);
   }

public:
   dm::Map<std::string, CombatAbilityPtr>    abilities_;
   dm::Boxed<om::EntityRef>                  target_;
};

static std::ostream& operator<<(std::ostream& os, const CombatAbility& o) { return (os << o.ToString()); }
static std::ostream& operator<<(std::ostream& os, const CombatAbilityList& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COMBAT_ABILITIES_H
