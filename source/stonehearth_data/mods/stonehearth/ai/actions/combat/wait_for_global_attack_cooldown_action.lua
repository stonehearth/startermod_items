local log = radiant.log.create_logger('combat')

local WaitForGlobalAttackCooldown = class()

WaitForGlobalAttackCooldown.name = 'wait for global attack cooldown'
WaitForGlobalAttackCooldown.does = 'stonehearth:combat:wait_for_global_attack_cooldown'
WaitForGlobalAttackCooldown.args = {}
WaitForGlobalAttackCooldown.version = 2
WaitForGlobalAttackCooldown.priority = 1

local epsilon = 0.000001

function WaitForGlobalAttackCooldown:start_thinking(ai, entity, args)
   if not entity:is_valid() then
      return
   end
   
   local state = stonehearth.combat:get_combat_state(entity)

   if not state:in_cooldown('global_attack_recovery') then
      ai:set_think_output()
      return
   end

   local end_time = state:get_cooldown_end_time('global_attack_recovery')
   local time_remaining = end_time - radiant.gamestate.now()

   -- need to add epsilon to account for rounding errors in the calendar timer
   local duration = time_remaining + epsilon

   self._timer = stonehearth.combat:set_timer(duration,
      function ()
         if not state:in_cooldown('global_attack_recovery') then
            ai:set_think_output()
         else
            -- this is a bug, or some other effect/spell increased our attack recovery time
            log:error('Aborting attack. Global_attack_recovery on %s still not expired.', entity)
            return
         end
      end
   )
end

function WaitForGlobalAttackCooldown:stop_thinking(ai, entity, args)
   if self._timer ~= nil then
      self._timer:destroy()
      self._timer = nil
   end
end

return WaitForGlobalAttackCooldown
