local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItemOnStructure = class()

PlaceItemOnStructure.name = 'place an item'
PlaceItemOnStructure.does = 'stonehearth:place_item_on_structure'
PlaceItemOnStructure.args = {
   item = Entity,       -- the item to place.  must have a 'stonehearth:entity_forms' component!
   location = Point3,   -- where to take it.  the ghost should already be there
   rotation = 'number', -- the rotation
   structure = Entity,  -- the structure.  note that this is also used to place items on the ground 'terrain structure'
   ignore_gravity = {      -- turn off gravity when placing
      type = 'boolean',
      default = false,
   }
}
PlaceItemOnStructure.version = 2
PlaceItemOnStructure.priority = 1

function PlaceItemOnStructure:start(ai, entity, args)
   ai:set_status_text('placing ' .. radiant.entities.get_name(args.item))
end

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItemOnStructure)
         :execute('stonehearth:pickup_item', {
               item = ai.ARGS.item
            })
         :execute('stonehearth:create_entity', {
               location = ai.ARGS.location,
               options = {
                  debug_text = 'place item on structure'
               }
            })
         :execute('stonehearth:set_hangable_adjacent', {
               item = ai.BACK(1).entity,
            })
         :execute('stonehearth:goto_entity', {
               entity = ai.BACK(2).entity,
            })
         :execute('stonehearth:place_carrying_on_structure_adjacent', {
               location = ai.ARGS.location,
               rotation = ai.ARGS.rotation,
               structure = ai.ARGS.structure,
               ignore_gravity = ai.ARGS.ignore_gravity,
            })
