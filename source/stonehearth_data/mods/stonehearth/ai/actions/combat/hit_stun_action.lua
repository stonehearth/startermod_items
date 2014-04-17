local HitStun = class()

HitStun.name = 'hit stun'
HitStun.does = 'stonehearth:compelled_behavior'
HitStun.args = {}
HitStun.version = 2
HitStun.priority = stonehearth.constants.priorities.compelled_behavior.HIT_STUN

function HitStun:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity

   self._think_output_set = false

   self:_register_events(ai, entity)
end

function HitStun:stop_thinking(ai, entity, args)
   if not self._running then
      self:_unregister_events()
   end
end

function HitStun:_register_events(ai, entity)
   if not self._registered then
      self._ai = ai
      self._entity = entity
      radiant.events.listen(entity, 'stonehearth:combat:hit_stun', self, self.on_hit_stun)
      self._registered = true
   end
end

function HitStun:_unregister_events()
   if self._registered then
      radiant.events.unlisten(self._entity, 'stonehearth:combat:hit_stun', self, self.on_hit_stun)
      self._ai = nil
      self._entity = nil
      self._registered = false
   end
end

function HitStun:on_hit_stun(args)
   if not self._think_output_set then
      self._ai:set_think_output()
      self._think_output_set = true
   else
      if self._running then
         self:_start_new_effect()
      end
   end
end

function HitStun:_start_new_effect()
   if self._hit_effect then
      self._hit_effect:set_finished_cb(nil)
      self._hit_effect:stop()
   end

   self._hit_effect = radiant.effects.run_effect(self._entity,  'combat_1h_hit' )
   self._hit_effect:set_finished_cb(
      function ()
         self._ai:resume()
      end
   )
end

function HitStun:start(ai, entity, args)
   self._running = true
end

function HitStun:run(ai, entity, args)
   self:_start_new_effect()
   ai:suspend('waiting for hit stun effect (or effect chain) to finish')
end

function HitStun:stop(ai, entity, args)
   self._running = false
   self:_unregister_events()
end

return HitStun
