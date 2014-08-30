local Entity = _radiant.om.Entity
local PickupItemOnTable = class()

PickupItemOnTable.name = 'pickup item on a table'
PickupItemOnTable.does = 'stonehearth:pickup_item_on_table'
PickupItemOnTable.args = {
   item = Entity,
}
PickupItemOnTable.version = 2
PickupItemOnTable.priority = 1

function PickupItemOnTable:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemOnTable)
         :execute('stonehearth:reserve_entity', { entity = ai.ARGS.item })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.item:get_component('mob'):get_parent()  })
         :execute('stonehearth:pickup_item_on_table_adjacent', { item = ai.ARGS.item })
