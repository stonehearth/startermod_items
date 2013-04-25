local Perform = class()

local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.perform', Perform)

function Perform:run(ai, entity, action, ...)
   local finished = false

   self._entity = entity
   self._action = action
   
   self._effect = ani_mgr:get_animation(entity):start_action(action, ...)
   ai:wait_until(function()
         return self._effect:finished()
      end)
   self._effect = nil
end

function Perform:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end


--[[
local Perform = class()

local md = require 'radiant.core.md'
local check = require 'radiant.core.check'

md:register_msg_handler('radiant.actions.perform', Perform)

Perform['radiant.md.create'] = function(self, action, ...)
   check:is_string(action)

   self.action = action
   self.args = {...}
   self.finished = false
end

-- Action messages 

Perform['radiant.ai.actions.start'] = function (self, entity)
   check:is_entity(entity)
   
   self.entity = entity
   md:listen(self.entity, 'radiant.animation', self)
   md:send_msg(self.entity, 'radiant.animation.start_action', self.action, unpack(self.args))
end

Perform['radiant.ai.actions.run'] = function (self)
   return self.finished
end

Perform['radiant.ai.actions.stop'] = function (self)
   md:send_msg(self.entity, 'radiant.animation.stop_action', self.action)
   md:unlisten(self.entity, 'radiant.animation', self)
end

Perform['radiant.animation.on_stop'] = function (self, action)
   if self.action == action then
      self.finished = true
   end
end
]]