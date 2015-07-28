local Entity = _radiant.om.Entity
local PickupItemTypeFromBackpack = class()

PickupItemTypeFromBackpack.name = 'pickup item type from backpack'
PickupItemTypeFromBackpack.does = 'stonehearth:pickup_item_type_from_backpack'
PickupItemTypeFromBackpack.args = {
   filter_fn = 'function',
   description = 'string',   
}
PickupItemTypeFromBackpack.think_output = {
   item = Entity
}
PickupItemTypeFromBackpack.version = 2
PickupItemTypeFromBackpack.priority = 2 -- at elevated priority so we unload our inventory first

function PickupItemTypeFromBackpack:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying then
      return
   end

   local filter_fn = args.filter_fn
   self._storage_component = entity:get_component('stonehearth:storage')
   if not self._storage_component then
      return
   end

   local items = self._storage_component:get_items()
   for id, item in pairs(items) do
      if filter_fn(item, { always_allow_stealing = true }) then
         self._item = item
         ai.CURRENT.carrying = item
         ai:protect_argument(item)
         ai:set_think_output({ item = item })
      end
   end
end

function PickupItemTypeFromBackpack:run(ai, entity, args)
   if stonehearth.ai:prepare_to_pickup_item(ai, entity, self._item) then
      return
   end
   assert(not radiant.entities.get_carrying(entity))

   local id = self._item:get_id()
   local item = self._storage_component:remove_item(id)
   if not item then
      ai:abort('failed to pull item out of backpack')
   end
   stonehearth.ai:pickup_item(ai, entity, self._item)
end

return PickupItemTypeFromBackpack
