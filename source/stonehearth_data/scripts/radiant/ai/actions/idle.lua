local Idle = class()

local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.idle', Idle)

function Idle:run(ai, entity)
   while true do
      local carrying = om:get_carrying(entity)
      local action = carrying and 'carry_idle' or 'idle'
      ai:execute('radiant.actions.perform', action)
   end
end


--[[
local Idle = class()

local log = require 'radiant.core.log'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'

md:register_msg_handler('radiant.behaviors.idle', Idle)
md:register_activity('radiant.activities.idle')

Idle['radiant.md.create'] = function(self, entity)
   log:debug('constructing new idle handler')
   self._entity = entity
   md:send_msg(entity, 'radiant.ai.register_need', self, true)
   md:send_msg(entity, 'radiant.ai.register_behavior', self, true)
end

-- Need messages

Idle['radiant.ai.needs.recommend_activity'] = function(self)
   return 1, md:create_activity('radiant.activities.idle')
end

-- Behavior messages
Idle['radiant.ai.behaviors.get_priority'] = function(self, activity)
   return activity.name == 'radiant.activities.idle' and 1 or 0
end

Idle['radiant.ai.behaviors.get_next_action'] = function(self, activity)
   local carrying = om:get_carrying(self._entity)
   return md:create_msg_handler('radiant.actions.perform', carrying and 'carry-idle' or 'idle')
end
]]