local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   for i = -10, 10 do
      self:place_item('stonehearth:small_boulder', i, 0)
   end

   for i = -5, 5 do
      self:place_item('stonehearth:small_boulder', 0, i)
   end

   local citizens = {
      self:place_citizen(-15, -15, 'footman', 'stonehearth:wooden_sword'),
      -- self:place_citizen(-10, -15, 'footman', 'stonehearth:wooden_sword'),
      -- self:place_citizen(-15, -10, 'footman', 'stonehearth:wooden_sword'),
      -- self:place_citizen(-10,  10, 'footman', 'stonehearth:wooden_sword'),
   }

   local enemies = {
      self:place_enemy(15, 15),
      -- self:place_enemy(10, 15),
      -- self:place_enemy(15, 10),
      -- self:place_enemy(10, 10),
   }

   self:equip_all(enemies)

   -- self:at(3000,
   --    function ()
   --       local attributes_component = citizens[1]:add_component('stonehearth:attributes')
   --       attributes_component:set_attribute('health', 0)
   --    end
   -- )
end

function CombatTest:equip_all(entities)
   for _, entity in pairs(entities) do
      self:equip_weapon(entity, 'stonehearth:wooden_sword')
   end
end

function CombatTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'mainHand')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function CombatTest:create_enemy_kingdom()
   local session = {
      player_id = 'game_master',
      faction = 'raider',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }

   stonehearth.inventory:add_inventory(session)
   self._enemy_population = stonehearth.population:add_population(session)
end

function CombatTest:place_enemy(x, z)
   local enemy = self._enemy_population:create_new_citizen()
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

return CombatTest
