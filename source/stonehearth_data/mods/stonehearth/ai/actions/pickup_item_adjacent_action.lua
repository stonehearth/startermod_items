local Entity = _radiant.om.Entity
local PickupItemAdjacent = class()

PickupItemAdjacent.name = 'pickup an item'
PickupItemAdjacent.does = 'stonehearth:pickup_item_adjacent'
PickupItemAdjacent.args = {
   item = Entity
}
PickupItemAdjacent.version = 2
PickupItemAdjacent.priority = 5

local log = radiant.log.create_logger('actions.pickup_item')

function PickupItemAdjacent:run(ai, entity, args)
   local item = args.item   
   radiant.check.is_entity(item)

   if radiant.entities.get_carrying(entity) ~= nil then
      ai:abort('cannot pick up another item while carrying one!')
   end
   if not radiant.entities.is_adjacent_to(entity, item) then
      ai:abort('%s is not adjacent to %s', entity, item)
   end

   log:info("%s picking up %s", tostring(entity), tostring(item))
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'carry_pickup'})
end

return PickupItemAdjacent
