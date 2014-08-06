local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local PlaceItemOnWallAdjacent = class()

PlaceItemOnWallAdjacent.name = 'place carrying on wall adjacent'
PlaceItemOnWallAdjacent.does = 'stonehearth:place_carrying_on_wall_adjacent'
PlaceItemOnWallAdjacent.args = {
   location = Point3,      -- where to take it.  the ghost should already be there   
   rotation = 'number',    -- rotation to apply to the item
   wall = Entity,          -- the wall
}
PlaceItemOnWallAdjacent.version = 2
PlaceItemOnWallAdjacent.priority = 2

function PlaceItemOnWallAdjacent:start_thinking(ai, entity, args)
   local item = ai.CURRENT.carrying
   if item then
      local iconic_component = item:get_component('stonehearth:iconic_form')
      if iconic_component then
         self._root_entity = iconic_component:get_root_entity()
         ai:set_think_output()
      end
   end
end

function PlaceItemOnWallAdjacent:run(ai, entity, args)

   assert(self._root_entity:is_valid())
   ai:execute('stonehearth:run_effect', { effect = 'work' })
   radiant.effects.run_effect(entity, '/stonehearth/data/effects/place_item')

   radiant.entities.remove_carrying(entity)
   radiant.entities.add_child(args.wall, self._root_entity)

   local position = args.location - radiant.entities.get_world_grid_location(args.wall)
   self._root_entity:add_component('mob')
                        :set_location_grid_aligned(position)
                        :turn_to(args.rotation)

end

return PlaceItemOnWallAdjacent
