local IdleTop = class()

IdleTop.name = 'stonehearth.actions.idle'
IdleTop.does = 'stonehearth.top'
IdleTop.priority = 1

function IdleTop:run(ai, entity)
   while true do
      ai:execute('stonehearth.idle')
   end
end

return IdleTop