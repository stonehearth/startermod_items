local constants = require('constants').construction

local Column = class()
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

-- called to initialize the component on creation and loading.
--
function Column:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
end

-- connect the column to the roof.  the next time you call layout, we'll make
-- sure to grow the column height all the way up to the roof
--
--    @param roof - the roof to attach to
--
function Column:connect_to_roof(roof)
   self._sv.roof = roof
   self.__saved_variables:mark_changed()
   return self
end

-- make the destination region match the shape of the column
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
function Column:begin_editing(entity)
   local region

   if entity then
      local other_column = entity:get_component('stonehearth:column')
      self._sv.roof = other_column._sv.roof
      self.__saved_variables:mark_changed()
      region = other_column:get_component('destination'):get_region():get()
   else
      region = self:_compute_column_shape()
   end

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
   local height
   if self._sv.roof then
      -- if we have a roof, make sure the column height goes all
      -- the way up to the top.
      local roof = self._sv.roof
      local location = radiant.entities.get_location_aligned(self._entity)
      local coord = location - radiant.entities.get_location_aligned(roof)
      local roof_region = roof:get_component('destination'):get_region():get()

      for cube in roof_region:each_cube() do
         if cube:contains(Point3(coord.x, cube.min.y, coord.z)) then
            local column_top = cube.min.y - coord.y 
            if not height or column_top < height then
               height = column_top
            end
         end
      end
   end
   -- if we didn't have a roof, just use the default storey height
   height = height or constants.STOREY_HEIGHT

   local shape = Region3()
   shape:add_unique_cube(Cube3(Point3(0, 0, 0), Point3(1, height, 1)))
   return shape
end

return Column
