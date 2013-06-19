local Footman = class()
local FootmanIntention = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local util = require 'radiant.core.util'
local ai_mgr = require 'radiant.ai.ai_mgr'

md:register_msg_handler("radiant.professions.footman", Footman)

Footman['radiant.md.create'] = function(self, entity)
   local rigs = {
      'module://stonehearth/professions/footman/footman.qb',
      'module://stonehearth/professions/footman/iron_armor.qb',
   }

   self._entity = entity
   self._intention = ai_mgr:add_intention(entity, 'radiant.intentions.attack_on_aggro')
   self._rig_name = rigs[math.random(1, #rigs)]
   om:add_rig_to_entity(entity, self._rig_name)
   
   local abilities = {
      'module://stonehearth/combat_abilities/1h_swing.txt',
      'module://stonehearth/combat_abilities/1h_forehand.txt',
      'module://stonehearth/combat_abilities/1h_forehand_spin.txt',
   }
   for _, ability_name in ipairs(abilities) do
      om:add_combat_ability(entity, ability_name)
   end   
end

Footman['radiant.md.destroy'] = function(self)
   self._intention:destroy()
   om:remove_rig_from_entity(self._entity, self._rig_name)
end
