#include "pch.h"
#include "lib/lua/register.h"
#include "lua_paperdoll_component.h"
#include "om/components/paperdoll.ridl.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaPaperdollComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterWeakGameObjectDerived<Paperdoll, Component>()
         .def("equip",                    &Paperdoll::SetSlot)
         .def("unequip",                  &Paperdoll::ClearSlot)
         .def("get_item_in_slot",         &Paperdoll::GetItemInSlot)
         .def("has_item_in_slot",         &Paperdoll::HasItemInSlot)
         .enum_("Slots") [
            value("MAIN_HAND",            Paperdoll::MAIN_HAND),
            value("WEAPON_SCABBARD",      Paperdoll::WEAPON_SCABBARD),
            value("NUM_SLOTS",            Paperdoll::NUM_SLOTS)
         ]
      ;
}
