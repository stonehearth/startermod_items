local TerrainType = require 'services.world_generation.terrain_type'

local TerrainInfo = class()

function TerrainInfo:__init()
   local grassland_info = {}
   grassland_info.step_size = 2
   grassland_info.mean_height = 18
   grassland_info.std_dev = 6
   grassland_info.max_height = 16
   self[TerrainType.grassland] = grassland_info

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

   assert((foothills_info.max_height - grassland_info.max_height) % foothills_info.step_size == 0)

   -- don't place means on quantization ("rounding") boundaries
   assert(grassland_info.mean_height % grassland_info.step_size ~= grassland_info.step_size/2)
   assert(foothills_info.mean_height % foothills_info.step_size ~= foothills_info.step_size/2)
   assert(mountains_info.mean_height % mountains_info.step_size ~= mountains_info.step_size/2)

   -- fancy mode quantization needs to be modified if step size is not even
   assert(foothills_info.step_size % 2 == 0)
   assert(mountains_info.step_size % 2 == 0)

   self.min_height = grassland_info.max_height - grassland_info.step_size

   -- tree lines
   --self.tree_line = foothills_info.max_height+mountains_info.step_size*2
end

function TerrainInfo:get_terrain_type(height)
   if height <= self[TerrainType.grassland].max_height then
      return TerrainType.grassland
   end

   if height <= self[TerrainType.foothills].max_height then
      return TerrainType.foothills
   end

   return TerrainType.mountains
end

return TerrainInfo
