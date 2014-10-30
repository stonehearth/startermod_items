local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   -- self:place_obstacles()

   self:at(1000,
      function ()
         self:place_units()
      end
   )
end

function CombatTest:place_obstacles()
   for i = -16, 10 do
      self:place_item('stonehearth:small_boulder', i, 0)
      if math.abs(i) <= 5 and i ~= 0 then
         self:place_item('stonehearth:small_boulder', 0, i)
      end
   end
end

function CombatTest:place_units()
   self._citizens = {
      self:place_citizen(-9, -15, 'stonehearth:jobs:footman'),
      self:place_citizen(-9, -15, 'stonehearth:jobs:footman'),
      self:place_citizen( -7, -15, 'stonehearth:jobs:carpenter'),
      self:place_citizen(  1, -15, 'stonehearth:jobs:weaver'),
      self:place_citizen(  9, -15, 'stonehearth:jobs:farmer'),
      self:place_citizen(  11, -15, 'stonehearth:jobs:trapper'),
   }

   self._enemies = {
      self:place_enemy( -9, 15, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy( -1, 15, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy(  7, 15, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy( 15, 15, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy( -9, 10, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy( -1, 10, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy(  7, 10, 'stonehearth:weapons:jagged_cleaver'),
      self:place_enemy( 15, 10, 'stonehearth:weapons:jagged_cleaver'),
   }
end

function CombatTest:create_enemy_kingdom()
   local session = {
      player_id = 'goblins',
   }

   stonehearth.inventory:add_inventory(session)
   stonehearth.town:add_town(session)
   self._enemy_population = stonehearth.population:add_population(session, 'stonehearth:kingdoms:goblin')
end

function CombatTest:place_enemy(x, z, weapon)
   local enemy = self._enemy_population:create_new_citizen()
   self:equip_weapon(enemy, weapon)
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

function CombatTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   radiant.entities.equip_item(entity, weapon)
end

function CombatTest:kill(entity)
   local attributes_component = entity:add_component('stonehearth:attributes')
   attributes_component:set_attribute('health', 0)
end

return CombatTest
