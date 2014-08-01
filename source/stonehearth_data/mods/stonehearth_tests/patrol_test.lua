local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Point2 = _radiant.csg.Point2

local PatrolTest = class(MicroWorld)

function PatrolTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   local footmen = {
      self:place_citizen(15, 15, 'footman', 'stonehearth:wooden_sword'),
      self:place_citizen(14, 14, 'footman', 'stonehearth:wooden_sword'),
      --self:place_citizen(13, 13, 'footman', 'stonehearth:wooden_sword'),
      --self:place_citizen(12, 12, 'footman', 'stonehearth:wooden_sword'),
   }

   -- local enemies = {
   --    self:place_enemy(-15, -15),
   -- }
   -- self:equip_all(enemies)

   local player_id = footmen[1]:add_component('unit_info'):get_player_id()
   self:place_stockpile_cmd(player_id, -13, -13, 5, 5)
   self:place_stockpile_cmd(player_id, -13,   8, 5, 5)
   self:place_stockpile_cmd(player_id,   8, -13, 5, 5)
   self:place_stockpile_cmd(player_id,   8,   8, 5, 5)
   self:place_stockpile_cmd(player_id,  -2,  -2, 5, 5)
end

function PatrolTest:equip_all(entities)
   for _, entity in pairs(entities) do
      self:equip_weapon(entity, 'stonehearth:wooden_sword')
   end
end

function PatrolTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   radiant.entities.equip_item(entity, weapon)
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
