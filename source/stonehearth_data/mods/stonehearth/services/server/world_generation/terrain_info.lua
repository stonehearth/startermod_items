local TerrainType = require 'services.server.world_generation.terrain_type'
local NonUniformQuantizer = require 'services.server.world_generation.math.non_uniform_quantizer'

local TerrainInfo = class()

function TerrainInfo:__init()
   -- block size constants
   self.tile_size = 256
   self.macro_block_size = 32
   self.feature_size = 16
   self.slice_size = 5
   assert(self.tile_size % self.macro_block_size == 0)
   assert(self.macro_block_size / self.feature_size == 2)

   -- elevation constants
   local plains_info = {}
   plains_info.step_size = 2
   plains_info.mean_height = 20
   plains_info.std_dev = 6
   plains_info.max_height = 20
   self[TerrainType.plains] = plains_info

   local foothills_info = {}
   foothills_info.step_size = 10
   foothills_info.mean_height = 36
   foothills_info.std_dev = 12
   foothills_info.max_height = 40
   self[TerrainType.foothills] = foothills_info

   local mountains_info = {}
   mountains_info.step_size = 15
   mountains_info.mean_height = 85
   mountains_info.std_dev = 80
   self[TerrainType.mountains] = mountains_info

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

   local max_mountains_steps = 10
   mountains_info.max_height = mountains_info.base_height + mountains_info.step_size*max_mountains_steps

   self.min_height = plains_info.valley_height
   self.max_height = mountains_info.max_height

   local centroids = self:_get_quantization_centroids()
   self.quantizer = NonUniformQuantizer(centroids)

   local mountain_centroids = self:_get_mountain_quantization_centroids()
   self.mountains_quantizer = NonUniformQuantizer(mountain_centroids)
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
   local plains_info = self[TerrainType.plains]
   local foothills_info = self[TerrainType.foothills]
   local mountains_info = self[TerrainType.mountains]
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
   local mountains_info = self[TerrainType.mountains]
   local centroids = {}
   local min, max, step_size

   -- make sure we have a quantization centroid at or below zero
   min = mountains_info.base_height % mountains_info.step_size - mountains_info.step_size
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
