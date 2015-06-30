local RunPutdownEffect = class()

local Point3 = _radiant.csg.Point3

RunPutdownEffect.name = 'run put down effect'
RunPutdownEffect.does = 'stonehearth:run_putdown_effect'
RunPutdownEffect.args = {
   location = Point3,
}
RunPutdownEffect.version = 2
RunPutdownEffect.priority = 1

local effect_names = {
   [0] = 'carry_putdown',
   [1] = 'carry_putdown_mid',
   [2] = 'carry_putdown_high',
}

function RunPutdownEffect:run(ai, entity, args)
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

return RunPutdownEffect
