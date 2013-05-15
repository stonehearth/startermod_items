local ShieldBlock = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.shield_block', ShieldBlock)

function ShieldBlock:run(ai, entity, attacker, script_info, start_time, end_time)
   check:is_entity(entity)

   self._entity = entity
   self._attacker = attacker
   md:listen(entity, 'radiant.combat.on_defend', self)

   -- wait for the attack to come
   self._effect = ani_mgr:get_animation(entity):start_action('idle_breathe')
   ai:wait_until(start_time)
   self._effect:stop()
   self._effect = nil

   -- run the block animation
   om:turn_to_face(entity, om:get_world_grid_location(attacker))
   
   -- go back to the idle stance forever.  the intention will lose its prioirty
   -- in due time
   ai:execute('radiant.actions.perform', script_info.effect)
   ai:wait_until(function() return false end)
end

function ShieldBlock:stop(ai, entity)
   md:unlisten(self._entity, 'radiant.combat.on_defend', self)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

ShieldBlock['radiant.combat.on_defend'] = function(self, segment, which, effects)
   for effect in effects:each() do
      if effect:get_type() == 'damage' then
         effect:mitigate('Block')
      end
   end
end
