local IdleSwayAction = class()

IdleSwayAction.name = 'sway'
IdleSwayAction.does = 'stonehearth:idle:bored'
IdleSwayAction.args = { }
IdleSwayAction.version = 2
IdleSwayAction.priority = 1
IdleSwayAction.preemptable = true

function IdleSwayAction:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'idle_sway'})
end

return IdleSwayAction
