local IdleBreatheAction = class()

IdleBreatheAction.name = 'breathe'
IdleBreatheAction.does = 'stonehearth:idle:breathe'
IdleBreatheAction.priority = 1

function IdleBreatheAction:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_breathe')
end

return IdleBreatheAction