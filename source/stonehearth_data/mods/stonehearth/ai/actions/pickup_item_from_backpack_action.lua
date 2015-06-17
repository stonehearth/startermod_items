local Entity = _radiant.om.Entity
local PickupItemFromBackpack = class()

PickupItemFromBackpack.name = 'pickup item from backpack'
PickupItemFromBackpack.does = 'stonehearth:pickup_item_from_backpack'
PickupItemFromBackpack.args = {
   item = Entity,
   backpack_entity = Entity
}
PickupItemFromBackpack.version = 2
PickupItemFromBackpack.priority = 1

function PickupItemFromBackpack:start_thinking(ai, entity, args)
   self._backpack_component = args.backpack_entity:get_component('stonehearth:backpack')

   if self._backpack_component and self._backpack_component:contains_item(args.item:get_id()) and not ai.CURRENT.carrying then
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

   radiant.entities.turn_to_face(entity, args.backpack_entity)

   local success = self._backpack_component:remove_item(args.item)

   if not success then
      ai:abort('item not found in backpack')
   end
   
   radiant.entities.pickup_item(entity, args.item)
 
   ai:execute('stonehearth:run_effect', { effect = 'carry_pickup' })
end

return PickupItemFromBackpack
