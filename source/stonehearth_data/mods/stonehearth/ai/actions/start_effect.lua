local StartEffect = class()
StartEffect.name = 'run effect'
StartEffect.does = 'stonehearth:start_effect'
StartEffect.args = {
   effect = 'string'           -- effect_name
}
StartEffect.version = 2
StartEffect.priority = 1

function StartEffect:run(ai, entity, args)
   local effect_name = args.effect
   radiant.effects.run_effect(entity, effect_name)
end

return StartEffect
