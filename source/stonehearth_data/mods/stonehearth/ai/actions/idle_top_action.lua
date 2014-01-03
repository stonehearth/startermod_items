local IdleTop = class()

IdleTop.name = 'top action for idle'
IdleTop.does = 'stonehearth:top'
IdleTop.version = 1
IdleTop.priority = 1

function IdleTop:run(ai, entity)
   while true do
      ai:execute('stonehearth:idle')
   end
end

return IdleTop