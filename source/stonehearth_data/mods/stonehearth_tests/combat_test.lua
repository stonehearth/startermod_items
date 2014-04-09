local MicroWorld = require 'lib.micro_world'
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local faction = 'civ'

   local enemy = self:place_citizen(14, 14, 'trapper')
   enemy:add_component('unit_info'):set_faction('raider')

   --self:place_item('stonehearth:small_boulder', 1, 1)

   local worker1 = self:place_citizen(-14, 14)
   local worker2 = self:place_citizen(14, -14, 'carpenter')

   --self:_equip_weapon(enemy)
   --self:_equip_weapon(worker1)
   --self:_equip_weapon(worker2)
end

function CombatTest:_equip_weapon(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:wooden_sword')
end

return CombatTest
