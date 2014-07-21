local RunDeathEffect = class()

RunDeathEffect.name = 'run death effect'
RunDeathEffect.does = 'stonehearth:run_death_effect'
RunDeathEffect.args = {}
RunDeathEffect.version = 2
RunDeathEffect.priority = 1

function RunDeathEffect:start_thinking(ai, entity, args)
   self._location = ai.CURRENT.location
   ai:set_think_output()
end

function RunDeathEffect:run(ai, entity, args)
   -- hide the entity
   radiant.terrain.remove_entity(entity)

   self._proxy_entity = radiant.entities.create_proxy_entity()
   radiant.terrain.place_entity(self._proxy_entity, self._location)

   self._effect = radiant.effects.run_effect(self._proxy_entity, '/stonehearth/data/effects/death/death.json')

   -- BUG: if the effect is interrupted / pre-empted this will never get called
   -- could set a timer longer than the effect and destroy then proxy then
   self._effect:set_finished_cb(
      function()
         if self._effect then
            ai:resume()
         end
      end)
   ai:suspend()
end

function RunDeathEffect:stop(ai, entity, args)
   self:_destroy_effect()
   radiant.entities.destroy_entity(self._proxy_entity)
end

function RunDeathEffect:_destroy_effect()
   if self._effect then
      self._effect:set_trigger_cb(nil)
                  :set_finished_cb(nil)
                  :stop()
      self._effect = nil
   end
end

return RunDeathEffect
