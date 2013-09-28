local PickupItemAction = class()

PickupItemAction.name = 'stonehearth.actions.pickup_item'
PickupItemAction.does = 'stonehearth.pickup_item'
PickupItemAction.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   ai: the AI for all entities
   entity: the person doing the picking up
   item: the thing to pick up
   parent: optional, the table-height parent that used to contain the thing.
           If nil, assume it's the ground or a stockpile on the ground

]]
function PickupItemAction:run(ai, entity, item, parent)
   radiant.check.is_entity(item)
   local carry_block = entity:get_component('carry_block')

   if carry_block:is_carrying() then
      local carrying = carry_block:get_carrying()
      if carrying:get_id() ~= item:get_id() then
         ai:abort('cannot pick up another item while carrying one!')
      end
      return
   end

   --go stand adjacent to the item.
   --if it's on an object, go stand next to the object
   --TODO: what if the parent is very large? We may need an offset too?
   local destination_target = item
   if parent then
     destination_target = parent
   end

   local obj_location = radiant.entities.get_world_grid_location(destination_target)
   if not radiant.entities.is_adjacent_to(entity, obj_location) then
      ai:execute('stonehearth.goto_entity', destination_target)
   end
   radiant.log.info("picking up item at %s", tostring(obj_location))

   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item, parent)
   if parent then
      --TODO: replace with carry putdown on table
      ai:execute('stonehearth.run_effect', 'carry_pickup')
   else
      ai:execute('stonehearth.run_effect', 'carry_pickup')
   end
end

return PickupItemAction