local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_population()

   -- for i = -10, 10 do
   --    self:place_item('stonehearth:small_boulder', i, 0)
   -- end

   local enemy1 = self:place_enemy(0, 2)

   local worker1 = self:place_citizen(-15, -15)
   local worker2 = self:place_citizen(-15,  15)
   -- local worker3 = self:place_citizen( 15, -15)
   -- local worker4 = self:place_citizen(-10, -10)
   -- local worker5 = self:place_citizen(-10,  10)
   -- local worker6 = self:place_citizen( 10, -10)
   -- local worker7 = self:place_citizen( 10,  10)
   -- local worker8 = self:place_citizen(  1,   1)

   --self:_equip_weapon(enemy1)
   --self:_equip_weapon(worker1)
   --self:_equip_weapon(worker2)

   -- self:at(3000,
   --    function ()
   --       local attributes_component = enemy:add_component('stonehearth:attributes')
   --       attributes_component:set_attribute('health', 0)
   --    end
   -- )
end

function CombatTest:equip_weapon(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:wooden_sword')
end

function CombatTest:create_enemy_population()
   local session = {
      player_id = 'game_master',
      faction = 'raider',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }

   self._enemy_population = stonehearth.population:add_population(session)
end

function CombatTest:place_enemy(x, z)
   local enemy = self._enemy_population:create_new_citizen()
   enemy:add_component('unit_info'):set_faction('raider')
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

return CombatTest
