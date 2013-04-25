local RunFromDanger = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_intention('radiant.intentions.run_from_danger', RunFromDanger)

function RunFromDanger:recommend_activity(entity)
   local top = om:get_target_table_top(entity, 'radiant.danger')
   if top and om:get_attribute(top.target, 'health') > 0 then
      return nil
      -- return top.value, { 'radiant.actions.run_from_danger', top.target }
   end
   return nil
end

--[[
local PanicNeed = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'

md:register_activity('radiant.activities.flee_foe')
md:register_msg_handler('radiant.needs.panic', PanicNeed)

PanicNeed['radiant.md.create'] = function(self, entity)   
   self._entity = entity
   md:send_msg(entity, 'radiant.ai.register_need', self, true)
end

PanicNeed['radiant.md.detach_to_entity'] = function(self, entity)
   md:send_msg(entity, 'radiant.ai.register_need', self, false)
end

PanicNeed['radiant.ai.needs.recommend_activity'] = function(self)
   local top = om:get_target_table_top(self._entity, 'radiant.panic')
   if top and util:is_entity(top.target) then
      return top.value, md:create_activity('radiant.activities.flee_foe', top.target)
   end
   return nil
end

]]