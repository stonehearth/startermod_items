local IdleLookAround = class()

IdleLookAround.name = 'look around'
IdleLookAround.does = 'stonehearth:idle:bored'
IdleLookAround.args = { }
IdleLookAround.version = 2
IdleLookAround.priority = 1

function IdleLookAround:run(ai, entity)
   ai:execute('stonehearth:run_effect', 'idle_look_around')
end

return IdleLookAround
