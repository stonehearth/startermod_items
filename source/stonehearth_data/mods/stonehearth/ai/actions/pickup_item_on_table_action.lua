local Entity = _radiant.om.Entity
local PickupItemOnTable = class()

PickupItemOnTable.name = 'pickup item on a table'
PickupItemOnTable.does = 'stonehearth:pickup_item_on_table'
PickupItemOnTable.args = {
   item = Entity,
}
PickupItemOnTable.version = 2
PickupItemOnTable.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(PickupItemOnTable)
         :when( function (ai)
               return ai.CURRENT.carrying == nil
            end )
         :execute('stonehearth:reserve_entity', { entity = ai.ARGS.item })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.item:get_component('mob'):get_parent()  })
         :execute('stonehearth:pickup_item_on_table_adjacent', { item = ai.ARGS.item })
