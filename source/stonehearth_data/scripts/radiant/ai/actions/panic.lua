local Panic = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.run_from_danger', Panic)

function Panic:run(ai, entity, run_from_entity)
   local origin = om:get_location(entity)
   local direction = origin - om:get_location(run_from_entity)
   direction:normalize()
   
   local dst = origin + direction * 10
   
   self._entity = entity
   self._aura = om:apply_aura(entity, 'radiant.aura.panic', run_from_entity)

   -- just drop what you're carrying right away (no animation)
   local carry_block = om:get_component(self._entity, 'carry_block')
   if carry_block and carry_block:is_carrying() then
      local item = carry_block:get_carrying()
      carry_block:set_carrying(nil)
      om:place_on_terrain(item, om:get_grid_location(self._entity))
   end

   self._effect = ani_mgr:get_animation(entity):start_action('run_panic')
   ai:execute('radiant.actions.goto_location', dst)
   self._effect:stop()
   self._effect = nil
end

function Panic:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

--[[
local PanicBehavior = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local check = require 'radiant.core.check'
local Coroutine = require 'radiant.core.coro'

md:register_msg_handler('radiant.behaviors.panic', PanicBehavior)

PanicBehavior['radiant.md.create'] = function(self, entity)
   self._entity = entity
   md:send_msg(entity, 'radiant.ai.register_behavior', self, true)
end

-- Behavior messages
PanicBehavior['radiant.ai.behaviors.get_priority'] = function(self, activity)
   if activity.name == 'radiant.activities.flee_foe' then
      return 10
   end
   return nil
end

PanicBehavior['radiant.ai.behaviors.start'] = function(self, activity)
   assert(not self._co)
   local flee_from = activity.args[1]
   check:is_entity(flee_from)
   
   self._co = Coroutine(self._entity)
   
   local origin = om:get_location(self._entity)
   local direction = origin - om:get_location(flee_from)  
   direction:normalize()
   
   local dst = origin + direction * 10
   
   self._aura = om:apply_aura(self._entity, 'radiant.aura.panic', flee_from)
   md:send_msg(self._entity, 'radiant.animation.start_action', 'run_panic')
   self._co:start(nil, function()
         -- just drop what you're carrying right away (no animation)
         local carry_block = om:get_component(self._entity, 'carry_block')
         if carry_block and carry_block:is_carrying() then
            local item = carry_block:get_carrying()
            carry_block:set_carrying(nil)
            om:place_on_terrain(item, om:get_grid_location(self._entity))
         end
      
      self._co:run_toward_point(dst, 0)
   end)
end

PanicBehavior['radiant.ai.behaviors.stop'] = function(self, activity)
   assert(self._aura)
   md:send_msg(self._entity, 'radiant.animation.stop_action', 'run_panic')
   --om:remove_aura(self._aura)
   self._co = nil
end

PanicBehavior['radiant.ai.behaviors.get_next_action'] = function(self, activity)
   return self._co and self._co:yield_next()
end
]]