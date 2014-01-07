local event_service = require 'services.event.event_service'

local IdleDespairAction = class()

IdleDespairAction.name = 'despair'
IdleDespairAction.does = 'stonehearth:idle:bored'
IdleDespairAction.version = 1
IdleDespairAction.priority = 1
IdleDespairAction.weight = 3

function IdleDespairAction:run(ai, entity)
   local name = radiant.entities.get_display_name(entity)
   event_service:add_entry(name .. ' is so hungry it feels like despair.', 'warning')

   for i = 1, 3 do
      ai:execute('stonehearth:run_effect', 'idle_despair')
   end
end

return IdleDespairAction
