local AttackOnAggro = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local log = require 'radiant.core.log'
local ai_mgr = require 'radiant.ai.ai_mgr'
ai_mgr:register_intention('radiant.intentions.attack_on_aggro', AttackOnAggro)

function AttackOnAggro:__init()
   local a = 1
end

function AttackOnAggro:recommend_activity(entity)
   local top = om:get_target_table_top(entity, 'radiant.aggro')
   if top then
      assert(util:is_entity(top.target))
      log:debug('AttackOnAggro intention: top of aggro table is %s - %d', tostring(top.target), top.value)
      if om:get_attribute(top.target, 'health') > 0 then
         log:debug('recommending radiant.actions.attack...')
         return 400000 + top.value, { 'radiant.actions.attack', top.target }
      end
   else
      log:debug('AttackOnAggro intention: top of aggro table is nil')
   end
end
