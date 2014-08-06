local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItemOnWall = class()

PlaceItemOnWall.name = 'place an item'
PlaceItemOnWall.does = 'stonehearth:place_item_on_wall'
PlaceItemOnWall.args = {
   item = Entity,       -- the item to place.  must have a 'stonehearth:entity_forms' component!
   location = Point3,   -- where to take it.  the ghost should already be there
   rotation = 'number', -- the rotation
   wall = Entity,       -- the wall
}
PlaceItemOnWall.version = 2
PlaceItemOnWall.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItemOnWall)
         :execute('stonehearth:pickup_item', {
               item = ai.ARGS.item
            })
         :execute('stonehearth:goto_location', {
               location = ai.ARGS.location,
               stop_when_adjacent = true,
            })
         :execute('stonehearth:place_carrying_on_wall_adjacent', {
               location = ai.ARGS.location,
               rotation = ai.ARGS.rotation,
               wall = ai.ARGS.wall
            })
