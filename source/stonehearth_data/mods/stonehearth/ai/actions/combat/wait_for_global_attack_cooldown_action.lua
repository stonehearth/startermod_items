local log = radiant.log.create_logger('combat')

local WaitForGlobalAttackCooldown = class()

WaitForGlobalAttackCooldown.name = 'wait for global attack cooldown'
WaitForGlobalAttackCooldown.does = 'stonehearth:combat:wait_for_global_attack_cooldown'
WaitForGlobalAttackCooldown.args = {}
WaitForGlobalAttackCooldown.version = 2
WaitForGlobalAttackCooldown.priority = 1

function WaitForGlobalAttackCooldown:start_thinking(ai, entity, args)
   local state = stonehearth.combat:get_combat_state(entity)

   if not state:in_cooldown('global_attack_recovery') then
      ai:set_think_output()
      return
   end

   local end_time = state:get_cooldown_end_time('global_attack_recovery')
   local time_remaining = end_time - radiant.gamestate.now()

   self._timer = stonehearth.combat:set_timer(time_remaining,
      function ()
         if not state:in_cooldown('global_attack_recovery') then
            ai:set_think_output()
         else
            -- this is a bug, or some other effect/spell increased our attack recovery time
            log:warning('Aborting attack. Global_attack_recovery on %s still not expired.', entity)
            ai:abort('Global_attack_recovery still not expired')
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
