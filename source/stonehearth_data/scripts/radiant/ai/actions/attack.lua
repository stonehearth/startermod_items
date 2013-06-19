local Attack = class()
local dkjson = require 'dkjson'

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.attack', Attack)

function Attack:run(ai, entity, target)
   check:is_entity(entity)
   check:is_entity(target)
   
   om:set_target(entity, target)
   while util:is_entity(target) and om:get_attribute(target, 'health') > 0 do
      -- pick an ability we can use
      local now = om:now()
      local next_ready_time

      local abilities = om:filter_combat_abilities(entity, function (ability)
            if ability:has_tag('attack') then
               local cooldown_ready = ability:get_cooldown_expire_time()
               if not next_ready_time or next_ready_time > cooldown_ready then
                  next_ready_time = cooldown_ready
               end
               return cooldown_ready < now
            end
         end)

      if #abilities == 0 then
         -- wait until we've bot something to do...
         next_ready_time = next_ready_time and next_ready_time or (now + 5000)
         -- xxx: start the "combat idle animation here"
         self:_idle(ai, entity, next_ready_time)
      else
         -- pick a random one...
         local ability = abilities[math.random(1, #abilities)]

         -- start it's cooldown.
         local cooldown = ability:get_cooldown()
         local delay = cooldown / 2 + math.random(1, cooldown / 2)
         ability:set_cooldown_expire_time(now + delay)

         -- run it
         local info = dkjson.decode(ability:get_json())
         ai:execute(ability:get_script_name(), target, info)
      end
   end   
end

function Attack:stop(ai, entity)
   om:set_target(entity, nil)
   if self._idle_effect then
      self._idle_effect:stop()
      self._idle_effect = nil
   end
end

function Attack:_idle(ai, entity, till)
   self._idle_effect = ani_mgr:get_animation(entity):start_action('idle_breathe')
   ai:wait_until(till)
   self._idle_effect:stop()
   self._idle_effect = nil
end
