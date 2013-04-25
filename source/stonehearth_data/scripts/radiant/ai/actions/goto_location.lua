local GotoLocation = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

-- xxx: merge this with follow path?
ai_mgr:register_action('radiant.actions.goto_location', GotoLocation)

function GotoLocation:run(ai, entity, location, travel_mode, close_to_distance)
   if not close_to_distance then
      close_to_distance = 0
   end

   local mi = om:get_movement_info(entity, travel_mode)   
   self._effect = ani_mgr:get_animation(entity):start_action(mi.effect_name)
   self._mover = native:create_goto_location(entity, mi.speed, location, close_to_distance)

   -- xxx: don't poll!
   ai:wait_until(function()
         local finished = self._mover:finished()
         if finished then
            return true
         end
      end)
      
   self._effect:stop()
   self._effect = nil
end

function GotoLocation:stop(ai, entity, path)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end
