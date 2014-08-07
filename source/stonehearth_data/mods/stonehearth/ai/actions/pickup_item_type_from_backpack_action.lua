local Entity = _radiant.om.Entity
local PickupItemTypeFromBackpack = class()

PickupItemTypeFromBackpack.name = 'pickup item type from backpack'
PickupItemTypeFromBackpack.does = 'stonehearth:pickup_item_type_from_backpack'
PickupItemTypeFromBackpack.args = {
   filter_fn = 'function'
}
PickupItemTypeFromBackpack.think_output = {
   item = Entity
}
PickupItemTypeFromBackpack.version = 2
PickupItemTypeFromBackpack.priority = 2 -- at elevated priority so we unload our inventory first

function PickupItemTypeFromBackpack:start_thinking(ai, entity, args)
   local filter_fn = args.filter_fn
   self._backpack_component = entity:get_component('stonehearth:backpack')
   if not self._backpack_component then
      return
   end

   local items = self._backpack_component:get_items()
   for id, item in pairs(items) do
      if filter_fn(item, entity) then
         self._item = item
         ai.CURRENT.carrying = item
         ai:set_think_output({ item = item })
      end
   end
end

function PickupItemTypeFromBackpack:run(ai, entity, args)
   self._backpack_component:remove_item(self._item)
   radiant.entities.pickup_item(entity, self._item)
end

return PickupItemTypeFromBackpack
