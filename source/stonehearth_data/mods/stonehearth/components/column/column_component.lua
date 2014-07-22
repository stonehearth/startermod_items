local constants = require('constants').construction

local Column = class()
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

-- called to initialize the component on creation and loading.
--
function Column:initialize(entity, json)
   self._entity = entity
end

-- make the rcs region match the shape of the column
--
function Column:layout()
   local height =  constants.STOREY_HEIGHT
   self._entity:get_component('destination')
                  :get_region()
                     :modify(function(cursor)
                           cursor:copy_region(self:_compute_column_shape())
                        end)
   return self
end

-- begin editing the column pointed to by `other_column`.  basically just
-- copy the shape and important variables in the save state
function Column:clone_from(entity)
   return self
end

-- begin editing the column pointed to by `other_column`.  basically just
-- copy the shape and important variables in the save state
function Column:begin_editing(entity)
   self._editing_region = _radiant.client.alloc_region()
   self._editing_region:modify(function(cursor)
         cursor:copy_region(self:_compute_column_shape())
      end)

   return self
end

-- return the 'editing region' for the column.  the editing region is a
-- region which fully covers the shape of the structure, minus all the
-- fixture and portals
--
function Column:get_editing_reserved_region()
   return self._editing_region
end

-- compute the shape of the column.
--
function Column:_compute_column_shape()
   local box = Cube3(Point3(0, 0, 0), Point3(1, constants.STOREY_HEIGHT, 1))

   -- if we're a client side authoring entity, we might not have a construction
   -- progress component.  that's ok!
   local cp = self._entity:get_component('stonehearth:construction_progress')
   if not cp then
      return Region3(box)
   end
   
   local building = cp:get_building_entity()
   return building:get_component('stonehearth:building')
                     :grow_local_box_to_roof(self._entity, box)
end

return Column
