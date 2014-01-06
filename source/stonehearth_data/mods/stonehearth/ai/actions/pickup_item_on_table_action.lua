local PickupItemOnTable = class()
local log = radiant.log.create_logger('actions.pickup_item')

PickupItemOnTable.name = 'pickup and item on the ground'
PickupItemOnTable.does = 'stonehearth:pickup_item_on_table'
PickupItemOnTable.version = 1
PickupItemOnTable.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   ai: the AI for all entities
   entity: the person doing the picking up
   item: the thing to pick up
   table: the table containing the item
]]
function PickupItemOnTable:run(ai, entity, item, table)
   radiant.check.is_entity(item)
   local carry_block = entity:get_component('carry_block')

   local parent = item:add_component('mob'):get_parent()
   if parent:get_id() ~= table:get_id() then
      ai:abort('item isn\'t on the table in pickup item on table action')
   end
   
   local carrying = carry_block:get_carrying()
   if carrying then
      if carrying:get_id() ~= item:get_id() then
         ai:abort('cannot pick up another item while carrying one!')
      end
      return -- already carrying it!  nothing to do...
   end   
  
   local obj_location = radiant.entities.get_world_grid_location(table)
   if not radiant.entities.is_adjacent_to(entity, obj_location) then
      -- ug!  not next to the item.  find a path to it... this is the
      -- slow way, but better than nothing...
      ai:execute('stonehearth:goto_entity', table)
   end
   
   log:info("picking up item on table.")
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   
   -- TODO: run the 'pickup item off table' effect
   ai:execute('stonehearth:run_effect', 'carry_pickup_from_table')
end

return PickupItemOnTable