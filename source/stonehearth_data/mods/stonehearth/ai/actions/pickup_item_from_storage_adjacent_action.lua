local Entity = _radiant.om.Entity
local PickupItemFromStorageAdjacent = class()

PickupItemFromStorageAdjacent.name = 'pickup item from storage adjacent'
PickupItemFromStorageAdjacent.does = 'stonehearth:pickup_item_from_storage_adjacent'
PickupItemFromStorageAdjacent.args = {
   item = Entity,
   storage = Entity
}
PickupItemFromStorageAdjacent.version = 2
PickupItemFromStorageAdjacent.priority = 1

function PickupItemFromStorageAdjacent:start_thinking(ai, entity, args)
   self._storage_component = args.storage:get_component('stonehearth:storage')

   if self._storage_component and self._storage_component:contains_item(args.item:get_id()) and not ai.CURRENT.carrying then
      ai.CURRENT.carrying = args.item
      ai:set_think_output()
   end
end

function PickupItemFromStorageAdjacent:run(ai, entity, args)
   local item = args.item   
   radiant.check.is_entity(item)

   if radiant.entities.get_carrying(entity) ~= nil then
      ai:abort('cannot pick up another item while carrying one!')
      return
   end

   if not radiant.entities.is_adjacent_to(entity, args.storage) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(args.storage))
   end

   radiant.entities.turn_to_face(entity, args.storage)

   local success = self._storage_component:remove_item(args.item:get_id())
   if not success then
      ai:abort('item not found in storage')
   end  
   radiant.entities.pickup_item(entity, args.item)
 
   ai:execute('stonehearth:run_effect', { effect = 'carry_pickup' })
end

return PickupItemFromStorageAdjacent
