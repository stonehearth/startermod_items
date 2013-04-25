local FollowPath = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.follow_path', FollowPath)

function FollowPath:run(ai, entity, path, travel_mode, close_to_distance)
   --[[
   local finished = false
   local travel_speed = speed and speed or 1
   self._entity = entity
   self._action = effect and effect or 'run'
   if om:get_carrying(entity) then
      self._action = "carry-" .. self._action
   end
   ]]
   
   if not close_to_distance then
      close_to_distance = 0
   end

   local mi = om:get_movement_info(entity, travel_mode)
   self._effect = ani_mgr:get_animation(entity):start_action(mi.effect_name)
   self._mover = native:create_follow_path(entity, mi.speed, path, close_to_distance)
   ai:wait_until(function()
         return self._mover:finished()
      end)
      
   self._effect:stop()
   self._effect = nil
end

function FollowPath:stop(ai, entity)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end

--[[
local FollowPath = class()

local md = require 'radiant.core.md'
local check = require 'radiant.core.check'

md:register_msg_handler('radiant.actions.FollowPath', FollowPath)

FollowPath['radiant.md.create'] = function(self, action, ...)
   check:is_string(action)

   self.action = action
   self.args = {...}
   self.finished = false
end

-- Action messages 

FollowPath['radiant.ai.actions.start'] = function (self, entity)
   check:is_entity(entity)
   
   self.entity = entity
   md:listen(self.entity, 'radiant.animation', self)
   md:send_msg(self.entity, 'radiant.animation.start_action', self.action, unpack(self.args))
end

FollowPath['radiant.ai.actions.run'] = function (self)
   return self.finished
end

FollowPath['radiant.ai.actions.stop'] = function (self)
   md:send_msg(self.entity, 'radiant.animation.stop_action', self.action)
   md:unlisten(self.entity, 'radiant.animation', self)
end

FollowPath['radiant.animation.on_stop'] = function (self, action)
   if self.action == action then
      self.finished = true
   end
end
]]