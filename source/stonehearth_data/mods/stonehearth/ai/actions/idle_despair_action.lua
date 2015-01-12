local IdleDespairAction = class()

IdleDespairAction.name = 'despair'
IdleDespairAction.does = 'stonehearth:idle:bored'
IdleDespairAction.args = {
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }
}
IdleDespairAction.version = 2
IdleDespairAction.priority = 1
IdleDespairAction.weight = 3

function IdleDespairAction:run(ai, entity)
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' is so hungry it feels like despair.', 'warning')

   for i = 1, 3 do
      ai:execute('stonehearth:run_effect', { effect = 'idle_despair' })
   end
end

return IdleDespairAction
