local Defend = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.defend', Defend)

function Defend:run(ai, entity, attacker, script_info, end_time)
   check:is_entity(entity)

   self._entity = entity
   self._attacker = attacker

   md:listen(entity, 'radiant.combat.on_defend', self)
  
--[[ 
   local ani = ani_mgr:get_animation(entity)
   local finish_duration = ani:get_effect_duration(script_info.finish)
   self._effect = ani:start_action(script_info.start)
   ai:wait_until(end_time - finish_duration)
   ]]
   
   ai:execute('radiant.actions.perform', script_info.start)
   ai:execute('radiant.actions.perform', script_info.effect)
   ai:execute('radiant.actions.perform', script_info.finish)
   ai:wait_until(function() return false end)
   
   --[[
   ai:execute('radiant.actions.perform', script_info.effect)
   --ai:execute('radiant.actions.perform', script_info.finish)
   ai:wait_until(function() return false end)
   ]]
end

function Defend:stop(ai, entity)
   md:unlisten(self._entity, 'radiant.combat.on_defend', self)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

Defend['radiant.combat.on_defend'] = function(self, entity, attacker)
end
