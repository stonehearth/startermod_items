local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   -- for i = -10, 10 do
   --    self:place_item('stonehearth:small_boulder', i, 0)
   --    if math.abs(i) <= 5 and i ~= 0 then
   --       self:place_item('stonehearth:small_boulder', 0, i)
   --    end
   -- end

   self:at(1000,
      function ()
         self:place_units()
      end
   )
end

function CombatTest:place_units()
   self._citizens = {
      self:place_citizen(-15, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen( -7, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen(  1, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen(  9, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
   }

   self._enemies = {
      self:place_enemy( -9, 15, 'stonehearth:wooden_sword'),
      self:place_enemy( -1, 15, 'stonehearth:wooden_sword'),
      self:place_enemy(  7, 15, 'stonehearth:wooden_sword'),
      self:place_enemy( 15, 15, 'stonehearth:wooden_sword'),
   }

   -- local enemy = self._enemy_population:create_new_citizen()
   -- radiant.terrain.place_entity(enemy, Point3(-9, 1, 15))
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

function CombatTest:place_enemy(x, z, weapon)
   local enemy = self._enemy_population:create_new_citizen()
   self:equip_weapon(enemy, weapon)
   self:inject_enemy_observer(enemy)
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

function CombatTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function CombatTest:inject_enemy_observer(entity)
   stonehearth.ai:inject_ai(entity, { observers = { "stonehearth:observers:enemy_observer" }})
end

function CombatTest:kill(entity)
   local attributes_component = entity:add_component('stonehearth:attributes')
   attributes_component:set_attribute('health', 0)
end

return CombatTest
