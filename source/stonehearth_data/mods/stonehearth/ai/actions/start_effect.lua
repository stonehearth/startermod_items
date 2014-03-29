local StartEffect = class()
StartEffect.name = 'run effect'
StartEffect.does = 'stonehearth:start_effect'
StartEffect.args = {
   effect = 'string',   -- effect_name
   delay = {
      type = 'number',
      default = 0
   }
}
StartEffect.version = 2
StartEffect.priority = 1

function StartEffect:run(ai, entity, args)
   local effect_name = args.effect
   radiant.effects.run_effect(entity, effect_name, args.delay)
end

return StartEffect
