local Entity = _radiant.om.Entity
local PickupItemAdjacent = class()

PickupItemAdjacent.name = 'pickup an item'
PickupItemAdjacent.does = 'stonehearth:pickup_item'
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

   log:info("picking up item at %s", tostring(obj_location))
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth:run_effect', 'carry_pickup')
end

return PickupItemAdjacent
