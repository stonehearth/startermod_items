local Die = class()

local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.die', Die)

function Die:run(ai, entity, damage_type, damage_amount)
   ai:execute('radiant.actions.perform', '1h_downed')
   om:destroy_entity(entity)
end


--[[
local Die = class()

local log = require 'radiant.core.log'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'

md:register_msg_handler('radiant.behaviors.idle', Die)
md:register_activity('radiant.activities.idle')

Die['radiant.md.create'] = function(self, entity)
   log:debug('constructing new idle handler')
   self._entity = entity
   md:send_msg(entity, 'radiant.ai.register_need', self, true)
   md:send_msg(entity, 'radiant.ai.register_behavior', self, true)
end

-- Need messages

Die['radiant.ai.needs.recommend_activity'] = function(self)
   return 1, md:create_activity('radiant.activities.idle')
end

-- Behavior messages
Die['radiant.ai.behaviors.get_priority'] = function(self, activity)
   return activity.name == 'radiant.activities.idle' and 1 or 0
end

Die['radiant.ai.behaviors.get_next_action'] = function(self, activity)
   local carrying = om:get_carrying(self._entity)
   return md:create_msg_handler('radiant.actions.perform', carrying and 'carry-idle' or 'idle')
end
]]