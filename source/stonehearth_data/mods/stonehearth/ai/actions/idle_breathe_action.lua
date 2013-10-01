local IdleBreatheAction = class()

IdleBreatheAction.name = 'stonehearth.actions.idle_breathe'
IdleBreatheAction.does = 'stonehearth.idle.breathe'
IdleBreatheAction.priority = 1

function IdleBreatheAction:run(ai, entity)
   local carrying = radiant.entities.get_carrying(entity)
     --- xxx: do this with postures...
   local effect = carrying and 'carry_idle' or 'idle_breathe'
   ai:execute('stonehearth.run_effect', effect)
end

return IdleBreatheAction