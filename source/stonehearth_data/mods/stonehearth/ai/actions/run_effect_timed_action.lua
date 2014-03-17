local RunEffectTimedAction = class()
RunEffectTimedAction.name = 'run effect timed...'
RunEffectTimedAction.does = 'stonehearth:run_effect_timed'
RunEffectTimedAction.args = {
   effect = 'string',    -- effect name
   duration = 'string'   -- game time for the effect to run (see stonehearth.calender:set_timer for formatting)
}
RunEffectTimedAction.version = 2
RunEffectTimedAction.priority = 1

function RunEffectTimedAction:run(ai, entity, args)
   self._effect = radiant.effects.run_effect(entity, args.effect)
   self._timer = stonehearth.calendar:set_timer(args.duration, function()
      ai:resume()
      self._timer = nil
   end)
   ai:suspend()
end

function RunEffectTimedAction:stop()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

return RunEffectTimedAction
