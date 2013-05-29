local RunEffectAction = class()

RunEffectAction.name = 'stonehearth.actions.run_effect'
RunEffectAction.does = 'stonehearth.activities.run_effect'
RunEffectAction.priority = 1

function RunEffectAction:run(ai, entity, effect_name, ...)
   local finished = false

   self._effect = radiant.effects.run_effect(entity, effect_name, ...)

   -- call ai:schedule in some effect finished callback functinon so we don't
   -- have to poll.  in fact, wait_until(predicate) should just be removed.
   -- there should be 'wait until timeout expired' or 'suspend / resume'
   
   ai:wait_until(function()
         return self._effect:finished()
      end)
   self._effect = nil
end

function RunEffectAction:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

return RunEffectAction
