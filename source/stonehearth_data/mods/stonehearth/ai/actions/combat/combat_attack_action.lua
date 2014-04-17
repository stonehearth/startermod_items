local constants = require 'constants'
local Entity = _radiant.om.Entity

local CombatAttackAction = class()
CombatAttackAction.name = 'combat attack action'
CombatAttackAction.does = 'stonehearth:combat'
CombatAttackAction.args = {
   enemy = Entity
}
CombatAttackAction.version = 2
CombatAttackAction.priority = constants.priorities.combat.ACTIVE

function CombatAttackAction:__init()
   self._global_attack_recovery_cooldown = radiant.util.get_config('global_attack_recovery_cooldown', 1000)
end

function CombatAttackAction:start_thinking(ai, entity, args)
   local state = stonehearth.combat:get_combat_state(entity)

   if not state:in_cooldown('global_attack_recovery') then
      ai:set_think_output()
      return
   end

   local end_time = state:get_cooldown_end_time('global_attack_recovery')
   local time_remaining = end_time - radiant.gamestate.now()

   self._timer = stonehearth.combat:set_timer(time_remaining,
      function ()
         -- TODO: abort if target is not valid (out of range, not hostile, etc)
         if not args.enemy:is_valid() then
            ai:abort('enemy has been destroyed')
         end

         if not state:in_cooldown('global_attack_recovery') then
            ai:set_think_output()
         else
            ai:abort('global_attack_recovery still not expired, restart thinking')
         end
      end
   )
end

function CombatAttackAction:stop_thinking(ai, entity, args)
   if self._timer ~= nil then
      self._timer:destroy()
      self._timer = nil
   end
end

function CombatAttackAction:run(ai, entity, args)
   if not args.enemy:is_valid() then
      ai:abort('enemy has been destroyed')
      return
   end

   ai:execute('stonehearth:combat:attack', { target = args.enemy })
end

function CombatAttackAction:stop(ai, entity, args)
   local state = stonehearth.combat:get_combat_state(entity)

   state:start_cooldown('global_attack_recovery', self._global_attack_recovery_cooldown)
end

return CombatAttackAction
