local TerrainType = require 'services.world_generation.terrain_type'

local TerrainInfo = class()

function TerrainInfo:__init()
   local plains_info = {}
   plains_info.step_size = 2
   plains_info.mean_height = 18
   plains_info.std_dev = 6
   plains_info.max_height = 16
   self[TerrainType.plains] = plains_info

   local foothills_info = {}
   foothills_info.step_size = 8
   foothills_info.mean_height = 29
   foothills_info.std_dev = 8
   foothills_info.max_height = 32
   self[TerrainType.foothills] = foothills_info

   local mountains_info = {}
   mountains_info.step_size = 16
   mountains_info.mean_height = 80
   mountains_info.std_dev = 64
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
   plains_info.base_height = self.min_height
   foothills_info.base_height = plains_info.max_height
   mountains_info.base_height = foothills_info.max_height

   self.terrain_order = { TerrainType.plains, TerrainType.foothills, TerrainType.mountains }

   -- tree lines
   --self.tree_line = foothills_info.max_height+mountains_info.step_size*2
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
function TerrainInfo:get_terrain_code(height)
   local terrain_type = self:get_terrain_type(height)
   local base_height = self[terrain_type].base_height
   local step_size = self[terrain_type].step_size
   local step_number = (height - base_height) / step_size

   return string.format('%s_%d', terrain_type, step_number)
end

return TerrainInfo
