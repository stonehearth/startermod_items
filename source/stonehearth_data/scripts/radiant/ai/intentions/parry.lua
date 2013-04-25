local Parry = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.parry', Parry)

function Parry:run(ai, entity, attacker, script_info, start_time, end_time)
   check:is_entity(entity)
   
   log:info('running civ_1h_parry animation start:%d end:%d duration:%d', start_time, end_time, end_time - start_time)
   
   self._entity = entity
   self._attacker = attacker
   md:listen(entity, 'radiant.combat.on_defend', self)

   -- wait for the attack to come
   self._effect = ani_mgr:get_animation(entity):start_action('combat_idle')
   ai:wait_until(start_time)
   self._effect:stop()
   self._effect = nil
   
   om:turn_to_face(entity, om:get_world_grid_location(attacker))
   
   ai:execute('radiant.actions.perform', script_info.execute)
   ai:wait_until(function() return false end)
end

function Parry:stop(ai, entity)
   md:unlisten(self._entity, 'radiant.combat.on_defend', self)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

Parry['radiant.combat.on_defend'] = function(self, segment, which, effects)
   for effect in effects:each() do
      if effect:get_type() == 'damage' then
         effect:mitigate('Parry')
      end
   end
end
