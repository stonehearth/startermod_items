local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItemOnStructure = class()

PlaceItemOnStructure.name = 'place an item'
PlaceItemOnStructure.does = 'stonehearth:place_item_on_structure'
PlaceItemOnStructure.args = {
   item = Entity,       -- the item to place.  must have a 'stonehearth:entity_forms' component!
   location = Point3,   -- where to take it.  the ghost should already be there
   rotation = 'number', -- the rotation
   structure = Entity,  -- the structure
}
PlaceItemOnStructure.version = 2
PlaceItemOnStructure.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItemOnStructure)
         :execute('stonehearth:pickup_item', {
               item = ai.ARGS.item
            })
         :execute('stonehearth:goto_location', {
               reason = 'place item on structure',
               location = ai.ARGS.location,
               stop_when_adjacent = true,
            })
         :execute('stonehearth:place_carrying_on_structure_adjacent', {
               location = ai.ARGS.location,
               rotation = ai.ARGS.rotation,
               structure = ai.ARGS.structure
            })
