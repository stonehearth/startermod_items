local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local PlaceItemOnStructureAdjacent = class()

PlaceItemOnStructureAdjacent.name = 'place carrying on structure adjacent'
PlaceItemOnStructureAdjacent.does = 'stonehearth:place_carrying_on_structure_adjacent'
PlaceItemOnStructureAdjacent.args = {
   location = Point3,      -- where to take it.  the ghost should already be there   
   rotation = 'number',    -- rotation to apply to the item
   structure = Entity,     -- the structure
   ignore_gravity = {      -- turn off gravity when placing
      type = 'boolean',
      default = false,
   },
}
PlaceItemOnStructureAdjacent.version = 2
PlaceItemOnStructureAdjacent.priority = 2

function PlaceItemOnStructureAdjacent:start_thinking(ai, entity, args)
   local item = ai.CURRENT.carrying
   if item then
      local iconic_component = item:get_component('stonehearth:iconic_form')
      if iconic_component then
         self._root_entity = iconic_component:get_root_entity()
         ai:set_think_output()
      end
   end
end

function PlaceItemOnStructureAdjacent:run(ai, entity, args)
   assert(self._root_entity:is_valid())
   ai:execute('stonehearth:turn_to_face_point', { point = args.location })
   ai:execute('stonehearth:run_effect', { effect = 'work' })
   radiant.effects.run_effect(entity, '/stonehearth/data/effects/place_item')

   radiant.entities.remove_carrying(entity)
   radiant.entities.add_child(args.structure, self._root_entity)

   local position = args.location - radiant.entities.get_world_grid_location(args.structure)
   self._root_entity:add_component('mob')
                        :set_location_grid_aligned(position)
                        :turn_to(args.rotation)

   if args.ignore_gravity then
      entity:get_component('mob')
               :set_ignore_gravity(true)
   end
end

return PlaceItemOnStructureAdjacent
