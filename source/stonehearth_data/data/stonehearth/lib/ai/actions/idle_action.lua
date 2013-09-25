local IdleAction = class()

IdleAction.name = 'stonehearth.actions.idle'
IdleAction.does = 'stonehearth.activities.top'
IdleAction.priority = 1

function IdleAction:run(ai, entity)
   while true do      
      ai:execute('stonehearth.activities.idle')
   end
end

return IdleAction