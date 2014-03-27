local RunEffectAction = class()

local Entity = _radiant.om.Entity

RunEffectAction.name = 'run effect'
RunEffectAction.does = 'stonehearth:run_effect'
RunEffectAction.args = {
   effect = 'string',  -- effect_name
   delay = {
      type = 'number',
      default = 0
   },
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
   facing_entity = {
      type = Entity,
      default = stonehearth.ai.NIL,
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

   if args.facing_entity then
      radiant.entities.turn_to_face(entity, args.facing_entity)
   end

   local times = args.times

   local log = ai:get_log()
   for i = 1, times do
      log:debug('starting new effect "%s"', effect_name)
      self._effect = radiant.effects.run_effect(entity, effect_name, args.delay, nil, args.args)

      radiant.events.listen(entity, 'stonehearth:on_effect_finished', function ()
            if self._effect then
               self._effect:stop()
               log:debug('stopped effect "%s" and resuming', effect_name)
               self._effect = nil
               ai:resume('effect %s finished', effect_name)
            end
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

function RunEffectAction:stop(ai, entity, args)
   if self._effect then
      ai:get_log():debug('stopped effect "%s" in stop', args.effect)
      self._effect:stop()
      self._effect = nil
   end
   if self._trigger_fn then
      radiant.events.unlisten(entity, 'stonehearth:on_effect_trigger', self, self._on_effect_trigger)
   end
end

return RunEffectAction
