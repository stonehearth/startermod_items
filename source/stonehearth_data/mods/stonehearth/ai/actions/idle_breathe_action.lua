local IdleBreatheAction = class()

IdleBreatheAction.name = 'breathe'
IdleBreatheAction.does = 'stonehearth:idle:breathe'
IdleBreatheAction.version = 1
IdleBreatheAction.priority = 1

function IdleBreatheAction:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_breathe')
end

return IdleBreatheAction