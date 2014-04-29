local MicroWorld = require 'lib.micro_world'
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local faction = 'civ'

   local enemy = self:place_citizen(15, 15, 'trapper')
   enemy:add_component('unit_info'):set_faction('raider')

   for i = -10, 10 do
      self:place_item('stonehearth:small_boulder', i, 0)
   end

   local worker1 = self:place_citizen(-15, -15, 'carpenter')
   local worker2 = self:place_citizen(-15,  15)
   -- local worker3 = self:place_citizen( 15, -15)
   -- local worker4 = self:place_citizen(-10, -10)
   -- local worker5 = self:place_citizen(-10,  10)
   -- local worker6 = self:place_citizen( 10, -10)
   -- local worker7 = self:place_citizen( 10,  10)
   -- local worker8 = self:place_citizen(  1,   1)

   --self:_equip_weapon(enemy)
   --self:_equip_weapon(worker1)
   --self:_equip_weapon(worker2)

   -- self:at(3000,
   --    function ()
   --       local attributes_component = enemy:add_component('stonehearth:attributes')
   --       attributes_component:set_attribute('health', 0)
   --    end
   -- )
end

function CombatTest:_equip_weapon(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:wooden_sword')
end

return CombatTest
