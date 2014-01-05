local IdleSwayAction = class()

IdleSwayAction.name = 'sway'
IdleSwayAction.does = 'stonehearth:idle:bored'
IdleSwayAction.args = { }
IdleSwayAction.version = 2
IdleSwayAction.priority = 1

function IdleSwayAction:run(ai, entity)
   for i = 1, 2 do
      ai:execute('stonehearth:run_effect', 'idle_sway')
   end
end

return IdleSwayAction
