local RunEffectAction = class()

RunEffectAction.name = 'run effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.args = {
   effect = 'string' -- effect_name
}
RunEffectAction.version = 2
RunEffectAction.priority = 1

function RunEffectAction:run(ai, entity, args)
   local effect_name = args.effect
   -- create the effect and register a callback to resume the ai thread
   -- when it finishes.

   self._effect = radiant.effects.run_effect(entity, effect_name)
   self._effect:on_finished(function()
         self._effect:stop()
         self._effect = nil
         ai:resume('effect %s finished', effect_name)
      end)

   -- now suspend the thread.  we'll resume once the effect is over.
   ai:suspend()
   self._effect = nil 
end

function RunEffectAction:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

return RunEffectAction
