local event_service = stonehearth.events
local IdleDespairAction = class()

IdleDespairAction.name = 'despair'
IdleDespairAction.does = 'stonehearth:idle:bored'
IdleDespairAction.args = { }
IdleDespairAction.version = 2
IdleDespairAction.priority = 1
IdleDespairAction.weight = 3

function IdleDespairAction:run(ai, entity)
   local name = radiant.entities.get_display_name(entity)
   event_service:add_entry(name .. ' is so hungry it feels like despair.', 'warning')

   for i = 1, 3 do
      ai:execute('stonehearth:run_effect', { effect = 'idle_despair' })
   end
end

return IdleDespairAction
