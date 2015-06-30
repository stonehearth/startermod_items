local RunPickupEffect = class()

local Point3 = _radiant.csg.Point3

RunPickupEffect.name = 'run pick up effect'
RunPickupEffect.does = 'stonehearth:run_pickup_effect'
RunPickupEffect.args = {
   location = Point3,
}
RunPickupEffect.version = 2
RunPickupEffect.priority = 1

local effect_names = {
   [0] = 'carry_pickup',
   [1] = 'carry_pickup_mid',
   [2] = 'carry_pickup_high',
}

function RunPickupEffect:run(ai, entity, args)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   if not entity_location then
      return
   end

   local dy = (args.location - entity_location).y
   local effect_name = effect_names[dy]
   if not effect_name then
      ai:abort('%s at %s cannot reach %s', entity, entity_location, args.location)
   end

   ai:execute('stonehearth:run_effect', { effect = effect_name })
end

return RunPickupEffect
