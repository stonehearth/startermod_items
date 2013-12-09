local IdleLookAround = class()

IdleLookAround.name = 'look around'
IdleLookAround.does = 'stonehearth:idle:bored'
IdleLookAround.priority = 1

function IdleLookAround:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_look_around')
end

return IdleLookAround
