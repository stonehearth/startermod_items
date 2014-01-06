local PickupItemOnGround = class()
local log = radiant.log.create_logger('actions.pickup_item')

PickupItemOnGround.name = 'pickup an item on the ground'
PickupItemOnGround.does = 'stonehearth:pickup_item_on_ground'
PickupItemOnGround.version = 1
PickupItemOnGround.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   ai: the AI for all entities
   entity: the person doing the picking up
   item: the thing to pick up
]]
function PickupItemOnGround:run(ai, entity, item)
   if not item then
      ai:abort('no item specified in stonehearth:pickup_item_on_ground')
   end
   radiant.check.is_entity(item)
   local carry_block = entity:get_component('carry_block')

   local carrying = carry_block:get_carrying()
   if carrying then
      if carrying:get_id() ~= item:get_id() then
         ai:abort('cannot pick up another item while carrying one!')
      end
      return -- already carrying it!  nothing to do...b
   end

   local obj_location = radiant.entities.get_world_grid_location(item)
   if not radiant.entities.is_adjacent_to(entity, obj_location) then
      -- ug!  not next to the item.  find a path to it... this is the
      -- slow way, but better than nothing...
      ai:execute('stonehearth:goto_entity', item)
   end

   log:info("picking up item at %s", tostring(obj_location))
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth:run_effect', 'carry_pickup')
end

return PickupItemOnGround