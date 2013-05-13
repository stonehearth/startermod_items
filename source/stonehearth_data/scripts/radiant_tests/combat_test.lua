local gm = require 'radiant.core.gm'
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local MicroWorld = require 'radiant_tests.micro_world'

local CombatTest = class(MicroWorld)
gm:register_scenario('radiant.tests.combat_test', CombatTest)

CombatTest['radiant.md.create'] = function(self, bounds)
   self:create_world()
   
   for i = 1, 2 do
      local footman = self:place_citizen(4, -12 + i * 6, 'footman')  
      om:add_combat_ability(footman, 'module://stonehearth/combat_abilities/parry.txt')
      ai_mgr:add_intention(footman, 'radiant.intentions.combat_defense')
      local sword = om:create_entity('module://stonehearth/items/iron_sword')
      om:equip(footman, Paperdoll.MAIN_HAND, sword)
   end
   
   for i = 1, 2 do
      local goblin = self:place_entity(-4, -12 + i * 6, 'module://stonehearth/mobs/goblin_soldier')
      local shield = om:create_entity('module://stonehearth/items/cracked_wooden_buckler')
      local helmet = om:create_entity('module://stonehearth/items/studded_leather_helmet')
      ai_mgr:add_intention(goblin, 'radiant.intentions.combat_defense')
      
      om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_block.txt')
      om:add_combat_ability(goblin, 'module://stonehearth/combat_abilities/shield_swing.txt')
      local mace = om:create_entity('module://stonehearth/items/spiked_wooden_mace')
      om:equip(goblin, Paperdoll.MAIN_HAND, mace)
   
      om:wear(goblin, shield)
      om:wear(goblin, helmet)
      if i == 2 then
         om:set_attribute(goblin, 'health', 10)
      end
   end
   
   --om:turn_to_face(footman, om:get_world_grid_location(goblin))
   --om:turn_to_face(goblin, om:get_world_grid_location(footman))
   self:at(100, function() end)
end
