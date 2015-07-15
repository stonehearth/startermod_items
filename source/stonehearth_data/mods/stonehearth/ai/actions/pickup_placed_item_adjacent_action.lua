local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local PickupPlacedItemAdjacent = class()

PickupPlacedItemAdjacent.name = 'break down a placed item'
PickupPlacedItemAdjacent.does = 'stonehearth:pickup_item_adjacent'
PickupPlacedItemAdjacent.args = {
   item = Entity     -- the big version
}
PickupPlacedItemAdjacent.version = 2
PickupPlacedItemAdjacent.priority = 2

local log = radiant.log.create_logger('actions.pickup_item')

function PickupPlacedItemAdjacent:start_thinking(ai, entity, args)
   local item = args.item

   ai:monitor_carrying()
   if ai.CURRENT.carrying ~= nil then
      return
   end

   local entity_forms = item:get_component('stonehearth:entity_forms')
   if entity_forms == nil then
      return
   end

   self._iconic_entity = entity_forms:get_iconic_entity()
   if not self._iconic_entity then
      return
   end

   ai.CURRENT.carrying = self._iconic_entity
   ai:set_think_output()
end

-- consider writing this as a compound action
-- goto_entity can invoke the pathfinder which may freeze the entity if the path becomes blocked
function PickupPlacedItemAdjacent:run(ai, entity, args)
   local item = args.item
   
   if stonehearth.ai:prepare_to_pickup_item(ai, entity, item) then
      return
   end

   radiant.entities.turn_to_face(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'work' })
   
   local location = item:get_component('mob'):get_world_grid_location()

   -- remove the item entity from whatever its parent was.  this could
   -- best the terrain or a wall, depending on how we were placed
   local parent = item:get_component('mob'):get_parent()
   if parent then
      radiant.entities.remove_child(parent, item)
      radiant.entities.move_to_grid_aligned(item, Point3.zero)
      radiant.entities.turn_to(item, 0)
   end
   -- gravity may have been turned off when placed.  turn it back on
   item:add_component('mob')
            :set_ignore_gravity(false)

   local entity_forms_component = item:get_component('stonehearth:entity_forms')
   if entity_forms_component and entity_forms_component:is_placeable_on_wall() then
      stonehearth.ai:pickup_item(ai, entity, self._iconic_entity)
   else
      radiant.terrain.place_entity(self._iconic_entity, location)
   
      -- After breaking down a large item, we may not be adjacent to the location where
      -- the iconic form is placed. This code walks over to the iconic to pick it up, but
      -- is dangerous since we should not be executing the pathfinder during run.
      -- (e.g. we will lock the entity if the adjacent straddles an impassable boundary
      -- and we cannot easily path to the iconic.) If this becomes a problem, just 
      -- skip placing the iconic and pickup the item directly.
      ai:execute('stonehearth:goto_entity', { entity = self._iconic_entity })
      ai:execute('stonehearth:pickup_item_adjacent', { item = self._iconic_entity })   
   end
end

return PickupPlacedItemAdjacent
