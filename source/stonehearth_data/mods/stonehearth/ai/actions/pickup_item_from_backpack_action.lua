local Entity = _radiant.om.Entity
local PickupItemFromBackpack = class()

PickupItemFromBackpack.name = 'pickup item from backpack'
PickupItemFromBackpack.does = 'stonehearth:pickup_item_from_backpack'
PickupItemFromBackpack.args = {
   item = Entity
}
PickupItemFromBackpack.version = 2
PickupItemFromBackpack.priority = 1

function PickupItemFromBackpack:start_thinking(ai, entity, args)
   self._storage_component = entity:get_component('stonehearth:storage')
   if self._storage_component and self._storage_component:contains_item(args.item:get_id()) and not ai.CURRENT.carrying then
      ai.CURRENT.carrying = args.item
      ai:set_think_output()
   end
end

function PickupItemFromBackpack:run(ai, entity, args)
   local item = args.item   
   radiant.check.is_entity(item)

   if radiant.entities.get_carrying(entity) ~= nil then
      ai:abort('cannot pick up another item while carrying one!')
      return
   end

   local success = self._storage_component:remove_item(args.item:get_id())
   if not success then
      ai:abort('item not found in backpack')
   end   
   radiant.entities.pickup_item(entity, args.item)
end

return PickupItemFromBackpack
