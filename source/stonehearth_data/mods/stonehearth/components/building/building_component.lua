local constants = require('constants').construction

local Building = class()
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2

function Building:initialize(entity, json)
   self._entity = entity
end

function Building:calculate_floor_region()
   local floor_region = Region2()
   local ec = self._entity:get_component('entity_container')
   for _, entity in ec:each_child() do
      if stonehearth.build:is_blueprint(entity) and entity:get_component('stonehearth:floor') then
         local rgn = entity:get_component('destination'):get_region():get()
         for cube in rgn:each_cube() do
            local rect = Rect2(Point2(cube.min.x, cube.min.z),
                               Point2(cube.max.x, cube.max.z))
            floor_region:add_unique_cube(rect)
         end
      end
   end
   return floor_region   
end

return Building
