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
   radiant.effects.run_effect(self._proxy_entity, '/stonehearth/data/effects/death/death.json')
end

function RunDeathEffect:stop(ai, entity, args)
   radiant.entities.destroy_entity(self._proxy_entity)
end

return RunDeathEffect
