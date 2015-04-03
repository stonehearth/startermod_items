local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local DetachedRegionSet = class()

-- Takes a set of cubes and returns the set of detached (unconnected) regions.
function DetachedRegionSet:__init()
   self._regions = {}
end

function DetachedRegionSet:get_regions()
   return self._regions
end

function DetachedRegionSet:add_region(region)
   for cube in region:each_cube() do
      self:add_cube(cube)
   end
end

function DetachedRegionSet:add_cube(cube)
   local cube_region = Region3(cube)
   local adjacent_regions = self:_get_adjacent_regions(cube_region)
   local parent_region = next(adjacent_regions)

   if not parent_region then
      self._regions[cube_region] = cube_region
   else
      -- add the new region to the parent
      parent_region:add_region(cube_region)

      -- remove the region we're keeping from the adjacent_set
      adjacent_regions[parent_region] = nil

      for _, adjacent_region in pairs(adjacent_regions) do
         -- merge all the connected regions into the parent
         parent_region:add_region(adjacent_region)
         -- remove the region we just merged from the set
         self._regions[adjacent_region] = nil
      end
   end
end

function DetachedRegionSet:_get_adjacent_regions(region)
   local inflated_region = csg_lib.get_non_diagonal_xyz_inflated_region(region)
   local adjacent_regions = {}

   for _, region in pairs(self._regions) do
      if inflated_region:intersects_region(region) then
         adjacent_regions[region] = region
      end
   end

   return adjacent_regions
end

return DetachedRegionSet
