local constants = require('constants').construction

local Floor = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3


-- called to initialize the component on creation and loading.
--
function Floor:initialize(entity, json)
   self._entity = entity
end

-- adds the `box` in world coordinates to the floor
--    @param box - the box to add to the floor, in world coordinates
--
function Floor:add_box_to_floor(box)
   local building = self._entity:get_component('mob'):get_parent()
   local origin = radiant.entities.get_world_grid_location(building)
   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(c)
                           c:add_cube(box:translated(-origin))
                        end)         

   return self
end

-- merges the floor region of `old_floor` into this entity and destroys
-- the `old_floor`
--    @param old_floor - the floor to merge with.  will be destroyed once
--                       we've grabbed the region.
--
function Floor:merge_with(old_floor)
   assert(old_floor:get_component('stonehearth:floor'))
   assert(old_floor:add_component('mob'):get_parent() == self._entity:add_component('mob'):get_parent())

   local floor_location = radiant.entities.get_location_aligned(self._entity)
   local floor_offset = radiant.entities.get_location_aligned(old_floor) - floor_location
   local old_floor_region = old_floor:get_component('destination'):get_region()

   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(cursor)
                           for cube in old_floor_region:get():each_cube() do
                              cursor:add_unique_cube(cube:translated(floor_offset))
                           end
                        end)

   radiant.entities.destroy_entity(old_floor)
end

return Floor
