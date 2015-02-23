local RunDeathEffect = class()

RunDeathEffect.name = 'run death effect'
RunDeathEffect.does = 'stonehearth:run_death_effect'
RunDeathEffect.args = {}
RunDeathEffect.version = 2
RunDeathEffect.priority = 1

function RunDeathEffect:run(ai, entity, args)
   local proxy = radiant.entities.create_entity(nil, { debug_text = 'running death effect' })
   local location = radiant.entities.get_world_grid_location(entity)
   
   radiant.terrain.place_entity(proxy, location)

   local effect = radiant.effects.run_effect(proxy, '/stonehearth/data/effects/death/death.json')

   effect:set_finished_cb(
      function()
         radiant.entities.destroy_entity(proxy)   
      end)
end

return RunDeathEffect
