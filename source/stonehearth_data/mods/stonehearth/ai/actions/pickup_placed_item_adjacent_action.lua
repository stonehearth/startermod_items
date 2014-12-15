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
   
   radiant.entities.turn_to_face(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'work' })
   
   local location = item:get_component('mob'):get_world_grid_location()

   -- remove the item entity from whatever its parent was.  this could
   -- best the terrain or a wall, depending on how we were placed
   local parent = item:get_component('mob'):get_parent()
   if parent then

      radiant.entities.remove_child(parent, item)
      radiant.entities.move_to(item, Point3.zero)
   end
   -- gravity may have been turned off when placed.  turn it back on
   item:get_component('mob')
            :set_ignore_gravity(false)

   radiant.terrain.place_entity(self._iconic_entity, location)
   
   --sometimes the location of the parent object is not adjacent to the
   --entity. In that case, walk over to the placed item.
   ai:execute('stonehearth:goto_entity', { entity = self._iconic_entity })
   ai:execute('stonehearth:pickup_item_adjacent', { item = self._iconic_entity })   
end

return PickupPlacedItemAdjacent
