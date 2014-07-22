local HitStun = class()

HitStun.name = 'hit stun'
HitStun.does = 'stonehearth:compelled_behavior'
HitStun.args = {}
HitStun.version = 2
HitStun.priority = stonehearth.constants.priorities.compelled_behavior.HIT_STUN

function HitStun:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._log = ai:get_log()

   self._think_output_set = false
   self._posture_set = false

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
      self._log:debug('listening for hit_stun messages')
      radiant.events.listen(entity, 'stonehearth:combat:hit_stun', self, self.on_hit_stun)
      self._registered = true
   end
end

function HitStun:_unregister_events()
   if self._registered then
      self._log:debug('unlistening for hit_stun messages')
      radiant.events.unlisten(self._entity, 'stonehearth:combat:hit_stun', self, self.on_hit_stun)
      self._ai = nil
      self._entity = nil
      self._registered = false
   end
end

function HitStun:on_hit_stun(args)
   self._log:spam('got hit stun message!')
   if not self._think_output_set then
      self._log:spam('calling set think output for hitstun')
      self._ai:set_think_output()
      self._think_output_set = true
   else
      -- if a hit stun message comes in while we were already doing a previous hitstun, just
      -- start the effect over.  this can happen when multiple people are ganging up on us!
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

   self._hit_effect = radiant.effects.run_effect(self._entity, 'combat_1h_hit')
   self._hit_effect:set_finished_cb(
      function ()
         if self._ai then
            self._ai:resume()
         end
      end
   )
end

function HitStun:start(ai, entity, args)
   self._running = true

   -- temporary code to determine if weapon should be wielded during hit stun animation
   -- TODO: figure out a better solution when we have a higher level notion of what an entity in combat means
   if stonehearth.combat:get_stance(entity) ~= 'passive' then
      radiant.entities.set_posture(entity, 'stonehearth:combat')
      self._posture_set = true
   end
end

function HitStun:run(ai, entity, args)
   self:_start_new_effect()
   ai:suspend('waiting for hit stun effect (or effect chain) to finish')
end

function HitStun:stop(ai, entity, args)
   self._running = false
   self:_unregister_events()

   if self._posture_set then
      radiant.entities.unset_posture(entity, 'stonehearth:combat')
      self._posture_set = false
   end
end

return HitStun
