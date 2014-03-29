local TerrainType = require 'services.server.world_generation.terrain_type'
local TerrainInfo = require 'services.server.world_generation.terrain_info'
local MathFns = require 'services.server.world_generation.math.math_fns'
local log = radiant.log.create_logger('world_generation')

local Terrain = _radiant.om.Terrain
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local construct_cube3 = _radiant.csg.construct_cube3
local Region3 = _radiant.csg.Region3

local BoulderGenerator = class()

function BoulderGenerator:__init(terrain_info, rng)
   self._terrain_info = terrain_info
   self._rng = rng

   self._boulder_size = {
      [TerrainType.plains]    = { min = 1, max = 3 },
      [TerrainType.foothills] = { min = 3, max = 6 },
      [TerrainType.mountains] = { min = 4, max = 9 }
   }
end

function BoulderGenerator:_create_boulder(x, y, elevation)
   local terrain_info = self._terrain_info
   local terrain_type = terrain_info:get_terrain_type(elevation)
   local step_size = terrain_info[terrain_type].step_size
   local boulder_region = Region3()
   local boulder_center = Point3(x, elevation, y)
   local i, j, half_width, half_length, half_height, boulder, chunk

   half_width, half_length, half_height = self:_get_boulder_dimensions(terrain_type)

   boulder = Cube3(Point3(x-half_width, elevation-2, y-half_length),
                         Point3(x+half_width, elevation+half_height, y+half_length),
                         Terrain.BOULDER)

   boulder_region:add_cube(boulder)

   -- take out a small chip from each corner of larger boulders
   local avg_length = MathFns.round((2*half_width + 2*half_length) * 0.5)
   if avg_length >= 6 then
      local chip_size = MathFns.round(avg_length * 0.15)
      local chip

      for j=-1, 1, 2 do
         for i=-1, 1, 2 do
            chip = self:_get_boulder_chip(i, j, chip_size, boulder_center,
                                          half_width, half_height, half_length)
            boulder_region:subtract_cube(chip)
         end
      end
   end

   -- remove a big chunk from one corner
   chunk = self:_get_boulder_chunk(boulder_center,
                                   half_width, half_height, half_length)
   boulder_region:subtract_cube(chunk)

   return boulder_region
end

-- dimensions are distances from the center
function BoulderGenerator:_get_boulder_dimensions(terrain_type)
   local rng = self._rng
   local size, half_length, half_width, half_height, aspect_ratio

   size = self._boulder_size[terrain_type]
   half_width = rng:get_int(size.min, size.max)

   half_height = half_width+1 -- make boulder look like its sitting slightly above ground
   half_length = half_width
   aspect_ratio = rng:get_gaussian(1, 0.15)

   if rng:get_real(0, 1) <= 0.50 then
      half_width = MathFns.round(half_width*aspect_ratio)
   else
      half_length = MathFns.round(half_length*aspect_ratio)
   end

   return half_width, half_length, half_height
end

function BoulderGenerator:_get_boulder_chip(sign_x, sign_y, chip_size, boulder_center, half_width, half_height, half_length)
   local corner1 = boulder_center + Point3(sign_x * half_width,
                                           half_height,
                                           sign_y * half_length)
   local corner2 = corner1 + Point3(-sign_x * chip_size,
                                    -chip_size,
                                    -sign_y * chip_size)
   return construct_cube3(corner1, corner2, 0)
end

function BoulderGenerator:_get_boulder_chunk(boulder_center, half_width, half_height, half_length)
   local rng = self._rng
   -- randomly find a corner (in cube space, not region space)
   local sign_x = rng:get_int(0, 1)*2 - 1
   local sign_y = rng:get_int(0, 1)*2 - 1

   local corner1 = boulder_center + Point3(sign_x * half_width,
                                           half_height,
                                           sign_y * half_length)

   -- length of the chunk as a percent of the length of the boulder
   local chunk_length_percent = rng:get_int(1, 2) * 0.25
   local chunk_depth = math.floor(half_height*0.5)
   local chunk_size

   -- pick an edge for the chunk
   local corner2
   if rng:get_real(0, 1) <= 0.50 then
      -- rounding causes some 75% chunks to look odd (the remaining slice is too narrow)
      -- so floor instead, but make it non-zero so we always have a chunk
      chunk_size = math.floor(2*half_width * chunk_length_percent)
      if chunk_size == 0 then
         chunk_size = 1
      end

      corner2 = corner1 + Point3(-sign_x * chunk_size,
                                 -chunk_depth,
                                 -sign_y * chunk_depth)
   else
      chunk_size = math.floor(2*half_length * chunk_length_percent)
      if chunk_size == 0 then
         chunk_size = 1
      end

      corner2 = corner1 + Point3(-sign_x * chunk_depth,
                                 -chunk_depth,
                                 -sign_y * chunk_size)
   end

   return construct_cube3(corner1, corner2, 0)
end

return BoulderGenerator
