 constants = require('constants').construction

local Building = class()
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2

function Building:initialize(entity, json)
   self._entity = entity
end

function Building:calculate_floor_region()
   local floor
   local floor_region = Region2()
   local ec = self._entity:get_component('entity_container')
   for _, entity in ec:each_child() do
      if stonehearth.build:is_blueprint(entity) and entity:get_component('stonehearth:floor') then
         floor = entity
         local rgn = entity:get_component('destination'):get_region():get()
         for cube in rgn:each_cube() do
            local rect = Rect2(Point2(cube.min.x, cube.min.z),
                               Point2(cube.max.x, cube.max.z))
            floor_region:add_unique_cube(rect)
         end
      end
   end

   -- we need to construct the most optimal region possible.  since floors are colored,
   -- the floor_region may be extremely complex (at worst, 1 cube per point!).  to get
   -- the best result, iterate through all the points in the region in an order in which
   -- the merge-on-add code in the region backend will build an extremly optimal region.
   local optimized_region = Region2()
   local bounds = floor_region:get_bounds()
   local pt = Point2()
   local try_merge = false
   for x=bounds.min.x,bounds.max.x do
      pt.x = x
      for y=bounds.min.y,bounds.max.y do
         pt.y = y
         if floor_region:contains(pt) then
            optimized_region:add_point(pt)
            if try_merge then
               optimized_region:optimize_by_merge()
            end
         else
            try_merge = true
         end
      end
   end

   return floor, optimized_region
end

return Building
