local Entity = _radiant.om.Entity
local PickupItemOnTableAdjacent = class()

PickupItemOnTableAdjacent.name = 'pickup item on table'
PickupItemOnTableAdjacent.does = 'stonehearth:pickup_item_on_table_adjacent'
PickupItemOnTableAdjacent.args = {
   item = Entity
}
PickupItemOnTableAdjacent.version = 2
PickupItemOnTableAdjacent.priority = 1

local log = radiant.log.create_logger('actions.pickup_item')

function PickupItemOnTableAdjacent:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying == nil then
      ai.CURRENT.carrying = args.item
      ai:set_think_output()
   end
end

function PickupItemOnTableAdjacent:run(ai, entity, args)
   local item = args.item   
   local table = item:get_component('mob'):get_parent()
   radiant.check.is_entity(item)

   if not table then
      ai:abort('item is not on a table!')
   end   
   if radiant.entities.get_carrying(entity) ~= nil then
      ai:abort('cannot pick up another item while carrying one!')
   end
   if not radiant.entities.is_adjacent_to(entity, table) then
      ai:abort('%s is not adjacent to table %s', entity, table)
   end

   log:info("%s picking up %s", tostring(entity), tostring(item))
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'carry_pickup_from_table'})
end

return PickupItemOnTableAdjacent
