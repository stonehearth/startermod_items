#include "pch.h"
#include "weapon_info.h"

using namespace ::radiant;
using namespace ::radiant::om;

luabind::scope WeaponInfo::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<WeaponInfo, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("get_range",          &WeaponInfo::GetRange)
         .def("get_damage_string",  &WeaponInfo::GetDamageString)
         .def("roll_damage",        &WeaponInfo::RollDamage)
      ;
}

std::string WeaponInfo::GetDamageString() const
{
   return *damage_;
}

std::string WeaponInfo::ToString() const
{
   ostringstream os;
   os << "(WeaponInfo id:" << GetObjectId() << " damage:" << *damage_ << " range:" << *range_ << ")";
   return os.str();
}

float WeaponInfo::GetRange() const
{
   return *range_;
}

void WeaponInfo::SetRange(float range)
{
   range_ = range;
}

void WeaponInfo::SetDamageString(std::string damage)
{
   damage_ = damage;
   computeDamageFn_ = [](om::EntityPtr e) -> int {
      return 10;
   };
}


int WeaponInfo::RollDamage(om::EntityRef against) const
{
   auto entity = against.lock();
   if (entity && computeDamageFn_) {
      return computeDamageFn_(entity);
   }
   return 0;
}


