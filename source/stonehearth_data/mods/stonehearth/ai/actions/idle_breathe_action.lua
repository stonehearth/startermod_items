local IdleBreatheAction = class()

IdleBreatheAction.name = 'breathe'
IdleBreatheAction.does = 'stonehearth:idle:breathe'
IdleBreatheAction.priority = 1

function IdleBreatheAction:run(ai, entity)
   local carrying = radiant.entities.get_carrying(entity)
     --- xxx: do this with postures...
   ai:execute('stonehearth:run_effect', 'idle_breathe')
end

return IdleBreatheAction