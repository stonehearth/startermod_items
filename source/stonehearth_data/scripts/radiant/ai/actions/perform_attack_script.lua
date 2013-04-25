local PerformAttackScript = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local AttackData = require 'radiant.ai.combat.attack_data'

ai_mgr:register_action('radiant.actions.perform_attack_script', PerformAttackScript)
md:register_msg('radiant.combat.on_defend_start')
md:register_msg('radiant.combat.on_defend_end')
md:register_msg('radiant.combat.on_attack')
md:register_msg('radiant.combat.on_defend')

function PerformAttackScript:run(ai, entity, target, effect_name)
   self._entity = entity
   self._target = target
   self._effect_name = effect_name

   md:send_msg(target, 'radiant.combat.on_defend_start', entity, effect_name)
   ai:execute('radiant.actions.perform', effect_name, self)
end

function PerformAttackScript:stop()
   md:send_msg(self._target, 'radiant.combat.on_defend_end', self._entity, self._effect_name)
end

PerformAttackScript['radiant.effect.frame_data.on_segment'] = function(self, which, segment)
   assert(which == 'start' or which == 'end')

   -- these messages are deferred.  either the entity or the foe may have been
   -- destroyed while the msg was in flight.  if so, just ignore it.
   if not util:is_entity(self._entity) or not util:is_entity(self._target) then
      return
   end
   
   -- compose strings like 'on_startup_phase_start' or 'on_execute_phase_end'
   local method = 'on_' .. segment.phase .. '_phase_' .. which
   local effects = AttackData(self._entity, self._target)
   self:_filter_combat_effects(self._entity, 'radiant.combat.on_attack', segment, which, effects)
   self:_filter_combat_effects(self._target, 'radiant.combat.on_defend', segment, which, effects)
   effects:resolve()
end

function PerformAttackScript:_filter_combat_effects(entity, msg, segment, which, effects)
   local slots = {
      Paperdoll.MAIN_HAND,
   }
   for _, slot in ipairs(slots) do
      local item = om:get_equipped(entity, slot)
      if util:is_entity(item) then
         md:sync_send_msg(item, msg, segment, which, effects)
      end
   end
   md:sync_send_msg(entity, msg, segment, which, effects)
end
