#ifndef _RADIANT_OM_WEAPON_INFO_H
#define _RADIANT_OM_WEAPON_INFO_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class WeaponInfo : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(WeaponInfo);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   WeaponInfo() { }

   float GetRange() const;
   void SetRange(float range);

   void SetDamageString(std::string damage);
   std::string GetDamageString() const;
   int RollDamage(om::EntityRef against) const;

   std::string ToString() const;

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("damage", damage_);
      AddRecordField("range", range_);
   }

private:
   static const char* rangeNames_;

private:
   std::function<int(om::EntityPtr)>   computeDamageFn_;
   dm::Boxed<std::string>              damage_;
   dm::Boxed<float>                    range_;
};

static std::ostream& operator<<(std::ostream& os, const WeaponInfo& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_WEAPON_INFO_H
