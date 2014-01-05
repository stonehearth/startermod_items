local RunEffectAction = class()

RunEffectAction.name = 'run effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.args = {
   'string' -- effect_name
}
RunEffectAction.version = 2
RunEffectAction.priority = 1

function RunEffectAction:run(ai, entity, effect_name)
   -- create the effect and register a callback to resume the ai thread
   -- when it finishes.
   self._effect = radiant.effects.run_effect(entity, effect_name)
   self._effect:on_finished(function()
         self._effect = nil
         ai:resume()
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
