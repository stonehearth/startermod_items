local RunEffectTimedAction = class()
RunEffectTimedAction.name = 'run effect timed...'
RunEffectTimedAction.does = 'stonehearth:run_effect_timed'
RunEffectTimedAction.args = {
   'string',   -- effect name
   'int'       -- hours to run the effect for
}
RunEffectTimedAction.version = 2
RunEffectTimedAction.priority = 1

-- XXX: THIS SHOUD NOT BE HOURS! !  CONVERT CALENDAR TIME TO TICKS!!!
function RunEffectTimedAction:run(ai, entity, effect_name, hours)
   self._effect = radiant.effects.run_effect(entity, 'sleep')
   self._timer = stonehearth.calendar:set_timer(hours, 0, 0, function()
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
