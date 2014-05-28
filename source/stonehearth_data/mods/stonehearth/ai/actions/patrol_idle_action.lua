local PatrolIdle = class()
PatrolIdle.name = 'patrol idle'
PatrolIdle.does = 'stonehearth:patrol:idle'
PatrolIdle.args = {}
PatrolIdle.version = 2
PatrolIdle.priority = 1

function PatrolIdle:run(ai, entity, args)
   ai:execute('stonehearth:run_effect', { effect = 'idle_look_around' })
end

return PatrolIdle
