local Entity = _radiant.om.Entity
local PickupItemAdjacent = class()

PickupItemAdjacent.name = 'pickup an item'
PickupItemAdjacent.does = 'stonehearth:pickup_item_adjacent'
PickupItemAdjacent.args = {
   item = Entity
}
PickupItemAdjacent.version = 2
PickupItemAdjacent.priority = 1

local log = radiant.log.create_logger('actions.pickup_item')

function PickupItemAdjacent:start_thinking(ai, entity, args)
   ai:monitor_carrying()
   if not ai.CURRENT.carrying then
      ai.CURRENT.carrying = args.item
      ai:set_think_output()
   end
end

function PickupItemAdjacent:run(ai, entity, args)
   local item = args.item   
   radiant.check.is_entity(item)

   if radiant.entities.get_carrying(entity) ~= nil then
      ai:abort('cannot pick up another item while carrying one!')
      return
   end
   if not radiant.entities.is_adjacent_to(entity, item) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(item))
      return
   end

   log:info("%s picking up %s", entity, item)
   local item_location = radiant.entities.get_world_grid_location(item)
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth:run_pickup_effect', { location = item_location })
end

return PickupItemAdjacent
