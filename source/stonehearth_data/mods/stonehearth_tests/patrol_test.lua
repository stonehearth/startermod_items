local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Point2 = _radiant.csg.Point2

local PatrolTest = class(MicroWorld)

function PatrolTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   self:place_citizen(0, 0, 'farmer')

   local footmen = {
      self:place_citizen(15, 15, 'footman', 'stonehearth:wooden_sword'),
      --self:place_citizen(13, 13, 'footman', 'stonehearth:wooden_sword'),
   }

   -- local enemies = {
   --    self:place_enemy(-15, -15),
   -- }
   -- self:equip_all(enemies)

   local player_id = footmen[1]:add_component('unit_info'):get_player_id()
   self:place_stockpile_cmd(player_id, -10, -10, 5, 5)
   self:place_stockpile_cmd(player_id, 5, 5, 5, 5)

   stonehearth.farming:create_new_field(
      { player_id = player_id, faction = radiant.entities.get_faction(footmen[1]) },
      Point3(-10, 1, 0),
      Point2(10, 10)
   )
end

function PatrolTest:equip_all(entities)
   for _, entity in pairs(entities) do
      self:equip_weapon(entity, 'stonehearth:wooden_sword')
   end
end

function PatrolTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function PatrolTest:create_enemy_kingdom()
   local session = {
      player_id = 'game_master',
      faction = 'raider',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }

   stonehearth.inventory:add_inventory(session)
   self._enemy_population = stonehearth.population:add_population(session)
end

function PatrolTest:place_enemy(x, z)
   local enemy = self._enemy_population:create_new_citizen()
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

return PatrolTest
