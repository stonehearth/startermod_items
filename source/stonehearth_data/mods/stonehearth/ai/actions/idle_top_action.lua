local IdleTop = class()

IdleTop.name = 'idle top'
IdleTop.does = 'stonehearth:top'
IdleTop.args = {}
IdleTop.version = 2
IdleTop.priority = stonehearth.constants.priorities.top.IDLE

function IdleTop:run(ai, entity)
   while true do
      ai:execute('stonehearth:idle')
   end
end

return IdleTop
