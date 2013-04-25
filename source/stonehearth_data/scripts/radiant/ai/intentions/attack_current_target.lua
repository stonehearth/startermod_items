--[[ local AttackCurrentTarget = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local ai_mgr = require 'radiant.ai.ai_mgr'
local cm = require 'radiant.ai.combat.combat_manager'
ai_mgr:register_intention('radiant.intentions.attack_current_target', AttackCurrentTarget)

function AttackCurrentTarget:recommend_activity(entity)
   -- if we're currently attacking someone, continue attacking them until
   -- the current attack sequence has finished.  this prevents us from ping-ponging
   -- between targets and ensures we continue the current attack animation after
   -- an opponent dies to the final blow
   local current_attack = cm:get_current_attack(entity)
   if not current_attack then
      self._current_activity = nil
   end
   
   -- choose a target
   local top = om:get_target_table_top(entity, 'radiant.aggro')
   if top and util:is_entity(top.target) then
      if om:get_attribute(top.target, 'health') > 0 then
         return top.value * 10, { 'radiant.actions.attack', top.target }
      end
   end
end
]]