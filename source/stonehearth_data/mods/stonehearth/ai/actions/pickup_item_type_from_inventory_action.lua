local Entity = _radiant.om.Entity
local PickupItemTypeFromInventory = class()

PickupItemTypeFromInventory.name = 'pickup item type from personal inventory'
PickupItemTypeFromInventory.does = 'stonehearth:pickup_item_type'
PickupItemTypeFromInventory.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromInventory.think_output = {
   item = Entity
}
PickupItemTypeFromInventory.version = 2
PickupItemTypeFromInventory.priority = 2 -- at elevated priority so we unload our inventory first

function PickupItemTypeFromInventory:start_thinking(ai, entity, args)
   local filter_fn = args.filter_fn
   self._inventory_component = entity:get_component('stonehearth:inventory')
   if not self._inventory_component then
      return
   end

   local items = self._inventory_component:get_items()
   for id, item in pairs(items) do
      if filter_fn(item, entity) then
         self._item = item
         ai.CURRENT.carrying = item
         ai:set_think_output({ item = item })
      end
   end
end

function PickupItemTypeFromInventory:run(ai, entity, args)
   self._inventory_component:remove_item(self._item)
   radiant.entities.pickup_item(entity, self._item)
end

return PickupItemTypeFromInventory
