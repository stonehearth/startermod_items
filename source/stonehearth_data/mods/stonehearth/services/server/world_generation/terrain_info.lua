local constants = require 'constants'
local NonUniformQuantizer = require 'services.server.world_generation.math.non_uniform_quantizer'
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local TerrainInfo = class()

function TerrainInfo:__init()
   -- block size constants
   self.tile_size = 256
   self.macro_block_size = 32
   self.feature_size = 16
   self.slice_size = constants.mining.Y_CELL_SIZE
   assert(self.tile_size % self.macro_block_size == 0)
   assert(self.macro_block_size / self.feature_size == 2)

   -- elevation constants
   local plains_info = {}
   plains_info.step_size = 2
   plains_info.mean_height = 40
   plains_info.std_dev = 6
   plains_info.max_height = 40
   self.plains = plains_info

   local foothills_info = {}
   foothills_info.step_size = 10
   foothills_info.mean_height = 56
   foothills_info.std_dev = 12
   foothills_info.max_height = 60
   self.foothills = foothills_info

   local mountains_info = {}
   mountains_info.step_size = 15
   mountains_info.mean_height = 105
   mountains_info.std_dev = 80
   self.mountains = mountains_info

   assert(plains_info.max_height % self.slice_size == 0)
   assert(foothills_info.max_height % self.slice_size == 0)

   assert(foothills_info.step_size % self.slice_size == 0)
   assert(mountains_info.step_size % self.slice_size == 0)

   -- don't place means on quantization ("rounding") boundaries
   -- do these asserts still make sense?
   assert(plains_info.mean_height % plains_info.step_size ~= plains_info.step_size/2)
   assert(foothills_info.mean_height % foothills_info.step_size ~= foothills_info.step_size/2)
   assert(mountains_info.mean_height % mountains_info.step_size ~= mountains_info.step_size/2)

   plains_info.valley_height = plains_info.max_height - plains_info.step_size
   plains_info.base_height = plains_info.valley_height - plains_info.step_size
   foothills_info.base_height = plains_info.max_height
   mountains_info.base_height = foothills_info.max_height

   local max_mountains_steps = 7
   mountains_info.max_height = mountains_info.base_height + mountains_info.step_size*max_mountains_steps

   self.min_height = plains_info.valley_height
   self.max_height = mountains_info.max_height

   local centroids = self:_get_quantization_centroids()
   self.quantizer = NonUniformQuantizer(centroids)

   local mountain_centroids = self:_get_mountain_quantization_centroids()
   self.mountains_quantizer = NonUniformQuantizer(mountain_centroids)
end

function TerrainInfo:get_terrain_type(height)
   if height <= self.plains.max_height then
      return 'plains'
   end

   if height <= self.foothills.max_height then
      return 'foothills'
   end

   return 'mountains'
end

-- takes quantized height values
function TerrainInfo:get_terrain_type_and_step(height)
   local terrain_type = self:get_terrain_type(height)
   local base_height = self[terrain_type].base_height
   local step_size = self[terrain_type].step_size
   local step_number = (height - base_height) / step_size

   return terrain_type, step_number
end

-- takes quantized height values
function TerrainInfo:get_terrain_code(height)
   local terrain_type, step = self:get_terrain_type_and_step(height)

   return string.format('%s_%d', terrain_type, step)
end

-- convert world coordinates to the index of the feature cell
function TerrainInfo:get_feature_index(x, y)
   local feature_size = self.feature_size
   return math.floor(x/feature_size),
          math.floor(y/feature_size)
end

-- convert world dimensions into feature dimensions
function TerrainInfo:get_feature_dimensions(width, length)
   local feature_size = self.feature_size
   return math.ceil(width/feature_size),
          math.ceil(length/feature_size)
end

function TerrainInfo:region_to_feature_space(region)
   local new_region = Region2()
   local num_rects = region:get_num_rects()
   local rect, new_rect

   -- use c++ base 0 array indexing
   for i=0, num_rects-1 do
      rect = region:get_rect(i)
      new_rect = self:rect_to_feature_space(rect)
      -- can't use add_unique_cube because of quantization to reduced coordinate space
      new_region:add_cube(new_rect)
   end

   return new_region
end

function TerrainInfo:rect_to_feature_space(rect)
   local feature_size = self.feature_size
   local min, max

   min = Point2(math.floor(rect.min.x/feature_size),
                math.floor(rect.min.y/feature_size))

   if rect:get_area() == 0 then
      max = min
   else
      max = Point2(math.ceil(rect.max.x/feature_size),
                   math.ceil(rect.max.y/feature_size))
   end

   return Rect2(min, max)
end

function TerrainInfo:_get_quantization_centroids()
   local plains_info = self.plains
   local foothills_info = self.foothills
   local mountains_info = self.mountains
   local centroids = {}
   local min, max, step_size
   
   min = plains_info.base_height + plains_info.step_size
   max = plains_info.max_height
   step_size = plains_info.step_size
   self:_append_lattice(centroids, min, max, step_size)

   min = foothills_info.base_height + foothills_info.step_size
   max = foothills_info.max_height
   step_size = foothills_info.step_size
   self:_append_lattice(centroids, min, max, step_size)

   min = mountains_info.base_height + mountains_info.step_size
   max = mountains_info.max_height
   step_size = mountains_info.step_size
   self:_append_lattice(centroids, min, max, step_size)

   return centroids
end

-- used to quantize the underground mountains
function TerrainInfo:_get_mountain_quantization_centroids()
   local mountains_info = self.mountains
   local centroids = {}
   local min, max, step_size

   min = mountains_info.base_height % mountains_info.step_size
   max = mountains_info.max_height
   step_size = mountains_info.step_size
   self:_append_lattice(centroids, min, max, step_size)

   return centroids
end

function TerrainInfo:_append_lattice(array, min, max, step_size)
   for value = min, max, step_size do
      table.insert(array, value)
   end
end

return TerrainInfo
