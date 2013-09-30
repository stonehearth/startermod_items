local IdleBreatheAction = class()

IdleBreatheAction.name = 'stonehearth.actions.idle_breathe'
IdleBreatheAction.does = 'stonehearth.idle'
IdleBreatheAction.priority = 1

function IdleBreatheAction:run(ai, entity)
   while true do      
      local carrying = radiant.entities.get_carrying(entity)
        --- xxx: do this with postures...
      local effect = carrying and 'carry_idle' or 'idle_breathe'
      ai:execute('stonehearth.run_effect', effect)
   end
end

return IdleBreatheAction