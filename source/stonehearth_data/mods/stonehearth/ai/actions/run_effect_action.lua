local RunEffectAction = class()

RunEffectAction.name = 'run effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.args = {
   effect = 'string', -- effect_name
   trigger_fn = {
      type = 'function',
      default = stonehearth.ai.NIL,
   },
   args = {
      type = 'table',
      default = stonehearth.ai.NIL,
   },
   times = {
      type = 'number',
      default = 1,
   },
}
RunEffectAction.version = 2
RunEffectAction.priority = 1

function RunEffectAction:run(ai, entity, args)
   local effect_name = args.effect
   -- create the effect and register a callback to resume the ai thread
   -- when it finishes.

   if args.trigger_fn then
      self._trigger_fn = args.trigger_fn
      radiant.events.listen(entity, 'stonehearth:on_effect_trigger', self, self._on_effect_trigger)
   end
   local times = args.times

   for i = 0, times do
      self._effect = radiant.effects.run_effect(entity, effect_name, nil, args.args)
      self._effect:on_finished(function()
            self._effect:stop()
            self._effect = nil
            ai:resume('effect %s finished', effect_name)
         end)
      ai:suspend()
   end
   
   self._effect = nil 
end

function RunEffectAction:_on_effect_trigger(e)
   local info = e.info
   local effect = e.effect

   if effect == self._effect then
      self._trigger_fn(info.info)
   end
end

function RunEffectAction:stop(ai, entity)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._trigger_fn then
      radiant.events.unlisten(entity, 'stonehearth:on_effect_trigger', self, self._on_effect_trigger)
   end
end

return RunEffectAction
