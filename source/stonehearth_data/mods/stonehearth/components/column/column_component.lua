local constants = require('constants').construction

local Column = class()
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- called to initialize the component on creation and loading.
--
function Column:initialize(entity, json)
   self._entity = entity
end

-- make the destination region match the shape of the column
--
function Column:layout()
   self._entity:get_component('destination')
                  :get_region()
                  :modify(function(cursor)
                        cursor:clear()
                        cursor:add_unique_cube(Cube3(Point3(0, 0, 0), Point3(1, constants.STOREY_HEIGHT, 1)))
                     end)
   return self
end

return Column
