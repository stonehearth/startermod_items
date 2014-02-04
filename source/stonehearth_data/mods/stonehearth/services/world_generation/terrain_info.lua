local TerrainType = require 'services.world_generation.terrain_type'
local NonUniformQuantizer = require 'services.world_generation.math.non_uniform_quantizer'

local TerrainInfo = class()

function TerrainInfo:__init()
   -- block size constants
   self.tile_size = 256
   self.macro_block_size = 32
   self.feature_size = 16
   assert(self.tile_size % self.macro_block_size == 0)
   assert(self.macro_block_size / self.feature_size == 2)

   -- elevation constants
   local plains_info = {}
   plains_info.step_size = 2
   plains_info.mean_height = 16
   plains_info.std_dev = 6
   plains_info.max_height = 16
   self[TerrainType.plains] = plains_info

   local foothills_info = {}
   foothills_info.step_size = 8
   foothills_info.mean_height = 29
   foothills_info.std_dev = 10
   foothills_info.max_height = 32
   self[TerrainType.foothills] = foothills_info

   local mountains_info = {}
   mountains_info.step_size = 16
   mountains_info.mean_height = 80
   mountains_info.std_dev = 80
   self[TerrainType.mountains] = mountains_info

   assert((foothills_info.max_height - plains_info.max_height) % foothills_info.step_size == 0)

   -- don't place means on quantization ("rounding") boundaries
   assert(plains_info.mean_height % plains_info.step_size ~= plains_info.step_size/2)
   assert(foothills_info.mean_height % foothills_info.step_size ~= foothills_info.step_size/2)
   assert(mountains_info.mean_height % mountains_info.step_size ~= mountains_info.step_size/2)

   -- fancy mode quantization needs to be modified if step size is not even
   assert(foothills_info.step_size % 2 == 0)
   assert(mountains_info.step_size % 2 == 0)

   self.min_height = plains_info.max_height - plains_info.step_size
   plains_info.base_height = self.min_height - plains_info.step_size -- essentially the water level
   foothills_info.base_height = plains_info.max_height
   mountains_info.base_height = foothills_info.max_height

   local centroids = self:_get_quantization_centroids()
   self.quantizer = NonUniformQuantizer(centroids)
end

function TerrainInfo:get_terrain_type(height)
   if height <= self[TerrainType.plains].max_height then
      return TerrainType.plains
   end

   if height <= self[TerrainType.foothills].max_height then
      return TerrainType.foothills
   end

   return TerrainType.mountains
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

function TerrainInfo:_get_quantization_centroids()
   local max_mountains_steps = 10
   local plains_info = self[TerrainType.plains]
   local foothills_info = self[TerrainType.foothills]
   local mountains_info = self[TerrainType.mountains]
   local centroids = {}
   local min, max, step_size
   
   min = plains_info.base_height + plains_info.step_size
   max = plains_info.max_height
   step_size = plains_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   min = foothills_info.base_height + foothills_info.step_size
   max = foothills_info.max_height
   step_size = foothills_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   min = mountains_info.base_height + mountains_info.step_size
   max = mountains_info.base_height + mountains_info.step_size*max_mountains_steps
   step_size = mountains_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   return centroids
end

return TerrainInfo
