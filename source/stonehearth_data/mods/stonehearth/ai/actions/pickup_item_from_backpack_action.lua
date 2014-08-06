local Entity = _radiant.om.Entity
local PickupItemFromBackpack = class()

PickupItemFromBackpack.name = 'pickup item from backpack'
PickupItemFromBackpack.does = 'stonehearth:pickup_item_from_backpack'
PickupItemFromBackpack.args = {
   item = Entity,
}
PickupItemFromBackpack.version = 2
PickupItemFromBackpack.priority = 1

function PickupItemFromBackpack:start_thinking(ai, entity, args)
   self._backpack_component = entity:get_component('stonehearth:backpack')

   if self._backpack_component and self._backpack_component:contains_item(args.item) then
      ai.CURRENT.carrying = args.item
      ai:set_think_output()
   end
end

function PickupItemFromBackpack:run(ai, entity, args)
   local success = self._backpack_component:remove_item(args.item)

   if not success then
      ai:abort('item not found in backpack')
   end
   
   radiant.entities.pickup_item(entity, args.item)
end

return PickupItemFromBackpack
