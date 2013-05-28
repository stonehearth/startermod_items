local IdleAction = class()

IdleAction.name = 'stonehearth.actions.idle'
IdleAction.does = 'stonehearth.activities.top'
IdleAction.priority = 1

function IdleAction:run(ai, entity)
   while true do      
      local carrying = radiant.entities.get_carrying(entity)
        --- xxx: do this with postures...
      local effect = carrying and 'carry_idle' or 'idle_breathe'
      ai:execute('stonehearth.activities.run_effect', effect)
   end
end

return IdleAction