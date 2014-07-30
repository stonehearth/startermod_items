local IdleSwayAction = class()

IdleSwayAction.name = 'sway'
IdleSwayAction.does = 'stonehearth:idle:bored'
IdleSwayAction.args = { }
IdleSwayAction.version = 2
IdleSwayAction.priority = 1
IdleSwayAction.weight = 1

function IdleSwayAction:start_thinking(ai, entity)
   if not ai.CURRENT.carrying then
		ai:set_think_output()
   end
end

function IdleSwayAction:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'idle_sway'})
end

return IdleSwayAction
