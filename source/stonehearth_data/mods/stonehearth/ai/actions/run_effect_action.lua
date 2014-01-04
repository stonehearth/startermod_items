local RunEffectAction = class()

RunEffectAction.name = 'stonehearth:actions:run_effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.priority = 1
RunEffectAction.version = 1

function RunEffectAction:run(ai, entity, effect_name, ...)
   local finished = false

   self._effect = radiant.effects.run_effect(entity, effect_name, ...)
   self._effect:on_finished(function()
         self._effect = nil
         ai:resume()
      end)
      
   ai:suspend()
end

function RunEffectAction:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

return RunEffectAction
